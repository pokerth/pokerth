// Minimal compatibility layer to replace mysql++ usage with Qt6::Sql
// This header provides a tiny subset of the mysqlpp API used in src/dbofficial
// and implements it on top of Qt6::Sql. It is intentionally minimal and
// only supports the operations used by the existing code.

#pragma once

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtSql/QSqlRecord>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>

namespace mysqlpp {

// Dummy option used in original code. We accept it but ignore specifics.
class SetCharsetNameOption {
public:
    explicit SetCharsetNameOption(const char* charset) : m_charset(charset) {}
    std::string m_charset;
};

// Simple string wrapper to imitate mysqlpp::String
struct String {
    String() {}
    explicit String(const std::string &s) : s(s) {}
    explicit String(const char* c) : s(c ? c : "") {}
    operator std::string() const { return s; }
    std::string s;
    bool is_null() const { return s.empty(); }
    void to_string(std::string &out) const { out = s; }
    operator int() const { try { return s.empty() ? 0 : std::stoi(s); } catch(...) { return 0; } }
    operator unsigned int() const { try { return s.empty() ? 0u : static_cast<unsigned int>(std::stoul(s)); } catch(...) { return 0u; } }
};

// Simple DateTime wrapper that converts to a string acceptable by SQL
struct DateTime {
    explicit DateTime(time_t t = 0) { setTime(t); }
    void setTime(time_t t) { dt = QDateTime::fromSecsSinceEpoch((qint64)t); }
    operator std::string() const { return dt.toString(Qt::ISODate).toStdString(); }
private:
    QDateTime dt;
};

// Forward declarations
class Connection;

// A small result set wrapper providing result[row][col] access
class StoreQueryResult {
public:
    struct Row {
        Row(std::vector<std::string> &r) : row(r) {}
        String operator[](size_t i) const {
            if (i < row.size()) return String(row[i]);
            return String("");
        }
        std::vector<std::string> &row;
        bool empty() const { return row.empty(); }
    };

    StoreQueryResult() : m_valid(false) {}
    void setValid(bool v) { m_valid = v; }
    bool operator()() const { return m_valid; }
    explicit operator bool() const { return m_valid; }

    void addRow(const std::vector<std::string> &r) { m_rows.emplace_back(r); }
    Row operator[](size_t i) { return Row(m_rows[i]); }
    size_t size() const { return m_rows.size(); }
    size_t num_rows() const { return m_rows.size(); }

private:
    bool m_valid;
    std::vector<std::vector<std::string>> m_rows;
};

// Simple quote manipulator used in original code
struct Quote {};
static Quote quote;

// Query builder / executor
class Query {
public:
    explicit Query(Connection *c = nullptr);

    // stream-like appenders
    Query &operator<<(const char *s) { append(std::string(s)); return *this; }
    Query &operator<<(const std::string &s) { append(s); return *this; }
    Query &operator<<(const Quote &) { m_quoteNext = true; return *this; }
    template<typename T>
    Query &operator<<(const T &v) { std::ostringstream tmp; tmp << v; append(tmp.str()); return *this; }

    bool exec();
    StoreQueryResult store();
    const char *error() const { return m_lastError.c_str(); }
    void reset() { m_ss.str(""); m_ss.clear(); m_lastError.clear(); m_quoteNext = false; }

private:
    void append(const std::string &s) {
        if (m_quoteNext) {
            m_ss << '\'' << escape(s) << '\'';
            m_quoteNext = false;
        } else {
            m_ss << s;
        }
    }
    std::string escape(const std::string &in) const {
        std::string out;
        out.reserve(in.size()*2);
        for (char c : in) {
            if (c == '\'') out.push_back('\\');
            out.push_back(c);
        }
        return out;
    }

    Connection *m_conn;
    std::ostringstream m_ss;
    bool m_quoteNext = false;
    std::string m_lastError;
};

// Lightweight connection wrapper using QSqlDatabase
class Connection {
public:
    Connection() : m_connected(false) {}
    // Some code constructs Connection(false) with mysql++ - accept and ignore
    explicit Connection(bool) : m_connected(false) {}

    bool connect(const char *dbName, const char *host, const char *user, const char *pwd) {
        std::lock_guard<std::mutex> l(m_mutex);
        // Use QMYSQL driver
        static int instance = 0;
        m_connName = QString("pokerth_dbofficial_%1").arg(++instance);
        m_db = QSqlDatabase::addDatabase("QMYSQL", m_connName);
        m_db.setHostName(host);
        m_db.setDatabaseName(dbName);
        m_db.setUserName(user);
        m_db.setPassword(pwd);
        // Try open
        if (!m_db.open()) {
            m_lastError = m_db.lastError().text().toStdString();
            m_connected = false;
            return false;
        }
        m_connected = true;
        return true;
    }

    void disconnect() {
        std::lock_guard<std::mutex> l(m_mutex);
        if (m_db.isValid() && m_db.isOpen()) m_db.close();
        QSqlDatabase::removeDatabase(m_connName);
        m_connected = false;
    }

    bool connected() const { return m_connected; }

    // ping(): Validate the connection is still alive by executing a trivial
    // query. Returns true if the connection is ok, false if it's gone.
    // This replaces mysql_ping() which is not available in Qt SQL.
    bool ping() {
        std::lock_guard<std::mutex> l(m_mutex);
        if (!m_db.isValid() || !m_db.isOpen()) {
            m_connected = false;
            return false;
        }
        QSqlQuery q(m_db);
        if (!q.exec("SELECT 1")) {
            m_connected = false;
            return false;
        }
        return true;
    }

    Query query() { return Query(this); }

    void set_option(SetCharsetNameOption *opt) {
        // Try to set connection character set by executing a SET NAMES
        if (opt && m_db.isValid() && m_db.isOpen()) {
            QSqlQuery q(m_db);
            q.exec(QString::fromStdString("SET NAMES '" + opt->m_charset + "'"));
        }
    }

    bool execSQL(const std::string &sql, std::string &errorOut, StoreQueryResult *out = nullptr) {
        std::lock_guard<std::mutex> l(m_mutex);
        if (!m_db.isValid() || !m_db.isOpen()) {
            errorOut = "No DB connection";
            m_connected = false;
            return false;
        }
        QSqlQuery q(m_db);
        bool ok = q.exec(QString::fromStdString(sql));
        if (!ok) {
            QSqlError err = q.lastError();
            errorOut = err.text().toStdString();
            // Native MySQL error codes indicating connection loss:
            //   2006 = CR_SERVER_GONE_ERROR
            //   2013 = CR_SERVER_LOST
            int nativeCode = err.nativeErrorCode().toInt();
            if (nativeCode == 2006 || nativeCode == 2013
                || !m_db.isOpen()) {
                m_connected = false;
            }
            return false;
        }
        if (out) {
            // fetch rows
            QSqlRecord rec = q.record();
            int cols = rec.count();
            while (q.next()) {
                std::vector<std::string> row;
                row.reserve(cols);
                for (int i=0;i<cols;++i) {
                    row.push_back(q.value(i).toString().toStdString());
                }
                out->addRow(row);
            }
            out->setValid(true);
        }
        return true;
    }

    std::string lastError() const { return m_lastError; }
    // mysql++ uses error() to return a C-string sometimes
    const char *error() const { return m_lastError.c_str(); }

private:
    mutable std::mutex m_mutex;
    QSqlDatabase m_db;
    QString m_connName;
    bool m_connected;
    std::string m_lastError;
};

// Query implementation
inline Query::Query(Connection *c) : m_conn(c) {}

inline bool Query::exec() {
    std::string sql = m_ss.str();
    m_lastError.clear();
    if (!m_conn) { m_lastError = "No connection"; return false; }
    bool ok = m_conn->execSQL(sql, m_lastError, nullptr);
    return ok;
}

inline StoreQueryResult Query::store() {
    StoreQueryResult r;
    std::string sql = m_ss.str();
    m_lastError.clear();
    if (!m_conn) { m_lastError = "No connection"; return r; }
    if (!m_conn->execSQL(sql, m_lastError, &r)) {
        r.setValid(false);
    }
    return r;
}

} // namespace mysqlpp
