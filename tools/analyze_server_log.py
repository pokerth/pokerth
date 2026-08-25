#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Analyse pokerth_server message logs and render a one-page SVG report.

Reads the server's "Player ... connected" lines and derives:
  * daily active registered accounts (+ linear trend)
  * new registrations per day, from the growth of the highest dbId seen
  * return rate of newly registered accounts (D+1 / D+3 / D+7)
  * how many days newcomers stay active, versus established players
  * the time-of-day profile, and whether new accounts cluster in the quiet hours

Every number in the output is computed from the log; nothing is hard-coded.

  tools/analyze_server_log.py server_messages.log --lang en
"""

import argparse
import collections
import datetime
import html
import math
import re
import statistics
import sys

MONTHS = {m: i + 1 for i, m in enumerate(
    ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'])}

CONNECT_RE = re.compile(
    r'^(\d{4})-(\w{3})-(\d{2}) (\d{2}):(\d{2}):(\d{2}) MSG: Player "(.*?)" '
    r'\(id:\d+, dbId:(\d+)\) connected')
GUEST_RE = re.compile(r'Guest\d+')

# The quiet-hours window is this many hours wide; its position is detected.
NIGHT_HOURS = 4
# Cap of the "active days" histogram; the last bucket is an overflow bucket.
HIST_MAX = 8


# --------------------------------------------------------------------------- #
# parsing
# --------------------------------------------------------------------------- #

Login = collections.namedtuple('Login', 'when name db')


def parse(path):
    logins, stamps = [], []
    with open(path, errors='replace') as fh:
        for line in fh:
            m = CONNECT_RE.match(line)
            if not m:
                continue
            when = datetime.datetime(int(m.group(1)), MONTHS[m.group(2)], int(m.group(3)),
                                     int(m.group(4)), int(m.group(5)), int(m.group(6)))
            logins.append(Login(when, m.group(7), int(m.group(8))))
            stamps.append(when)
    if not logins:
        sys.exit('no "Player ... connected" lines found in %s' % path)
    return logins, sorted(stamps)


# --------------------------------------------------------------------------- #
# metrics
# --------------------------------------------------------------------------- #

def analyse(logins, stamps, keep_edges=False, night=None):
    m = {}
    by_day = collections.defaultdict(list)
    for lg in logins:
        by_day[lg.when.date()].append(lg)

    log_days = sorted(by_day)
    m['log_first'], m['log_last'] = log_days[0], log_days[-1]
    # The first and last day of a log are almost always partial - they would
    # read as a slump at both ends of every chart.
    days = log_days if keep_edges else log_days[1:-1]
    if len(days) < 8:
        sys.exit('need at least 8 full days of log, got %d' % len(days))
    m['days'] = days
    m['total_logins'] = len(logins)

    # Highest dbId ever handed out is the registration counter; its growth is
    # the number of accounts created that day.
    high = {d: max(l.db for l in by_day[d]) for d in log_days}
    m['baseline_db'] = high[log_days[0]]
    # The first day of the log has no predecessor, so its delta is unknowable -
    # that slot stays None rather than being reported as zero growth.
    m['new_per_day'] = [None if i == 0 else high[d] - high[log_days[i - 1]]
                        for d in days for i in [log_days.index(d)]]
    known = [v for v in m['new_per_day'] if v is not None]
    m['new_total'] = sum(known)
    m['new_days'] = len(known)

    # Guests get a throwaway name per session, so only registered accounts can
    # be counted as people.
    active = []
    for d in days:
        active.append(len({l.db for l in by_day[d] if not GUEST_RE.fullmatch(l.name)}))
    m['active_per_day'] = active
    m['active_mean'] = statistics.mean(active)
    m['active_sd'] = statistics.pstdev(active)

    xs = list(range(len(active)))
    mx, my = statistics.mean(xs), m['active_mean']
    m['slope'] = (sum((x - mx) * (y - my) for x, y in zip(xs, active))
                  / sum((x - mx) ** 2 for x in xs))
    m['r'] = m['slope'] * statistics.pstdev(xs) / m['active_sd'] if m['active_sd'] else 0.0

    # Week-over-week registration rate: first seven days against the last seven.
    m['weeks'] = weeks = _week_blocks(days)
    m['week_means'] = [statistics.mean([v for v in m['new_per_day'][a:b + 1] if v is not None])
                       for a, b, _ in weeks]
    m['new_ratio'] = (m['week_means'][-1] / m['week_means'][0]) if m['week_means'][0] else 0.0

    # --- accounts registered inside the observed window
    seen_days = collections.defaultdict(set)
    for lg in logins:
        seen_days[lg.db].add(lg.when.date())
    newcomers = {db: sorted(ds) for db, ds in seen_days.items() if db > m['baseline_db']}
    m['new_seen'] = len(newcomers)

    m['retention'] = {}
    for window in (1, 3, 7):
        back = total = 0
        for arrival in newcomers.values():
            if (m['log_last'] - arrival[0]).days < window:
                continue  # not observed long enough to be able to return
            total += 1
            back += any(0 < (d - arrival[0]).days <= window for d in arrival)
        m['retention'][window] = (back, total)

    mature = [ds for ds in newcomers.values() if (m['log_last'] - ds[0]).days >= 7]
    m['hist'] = collections.Counter(min(len(ds), HIST_MAX) for ds in mature)
    m['hist_total'] = len(mature)
    m['newcomer_days'] = statistics.mean(len(ds) for ds in mature) if mature else 0.0
    established = [ds for db, ds in seen_days.items() if db <= m['baseline_db']]
    m['established_days'] = statistics.mean(len(ds) for ds in established)
    m['one_day_share'] = m['hist'][1] / m['hist_total'] if m['hist_total'] else 0.0

    # --- established players: first week against last week
    first_week = set(days[:7])
    last_week = set(days[-7:])
    m['week_a'], m['week_b'] = (days[0], days[6]), (days[-7], days[-1])
    was = {l.db for l in logins if l.db <= m['baseline_db'] and l.when.date() in first_week}
    now = {l.db for l in logins if l.db <= m['baseline_db'] and l.when.date() in last_week}
    m['base_before'], m['base_after'] = len(was), len(now)
    m['base_gone'], m['base_back'] = len(was - now), len(now - was)

    # --- time of day
    day_set = set(days)
    per_hour = collections.Counter()
    first_hour = {}
    for lg in logins:
        if lg.when.date() not in day_set:
            continue
        per_hour[lg.when.hour] += 1
        if lg.db > m['baseline_db'] and lg.db not in first_hour:
            first_hour[lg.db] = lg.when.hour
    arrivals = collections.Counter(first_hour.values())
    total_logins, total_new = sum(per_hour.values()), sum(arrivals.values())
    m['logins_per_hour'] = [per_hour[h] / len(days) for h in range(24)]
    m['hour_index'] = [(arrivals[h] / total_new) / (per_hour[h] / total_logins)
                       if per_hour[h] and total_new else 0.0 for h in range(24)]

    # Quiet window = the contiguous block of NIGHT_HOURS with the fewest logins,
    # unless the caller pinned one.
    if night is None:
        start = min(range(24 - NIGHT_HOURS + 1),
                    key=lambda h: sum(per_hour[i] for i in range(h, h + NIGHT_HOURS)))
        night = (start, start + NIGHT_HOURS)
    m['night'] = night
    m['night_login_share'] = sum(per_hour[h] for h in range(*m['night'])) / total_logins
    m['night_new_share'] = sum(arrivals[h] for h in range(*m['night'])) / total_new
    m['night_index'] = (m['night_new_share'] / m['night_login_share']
                        if m['night_login_share'] else 0.0)
    m['night_p'] = _binom_tail(total_new, sum(arrivals[h] for h in range(*m['night'])),
                               m['night_login_share'])

    # --- longest stretches without a single login, and where they sit
    gaps = sorted(((stamps[i + 1] - stamps[i]).total_seconds(), stamps[i])
                  for i in range(len(stamps) - 1))[-8:]
    m['gap_count'] = len(gaps)
    m['gap_min'] = min(g for g, _ in gaps) / 60
    m['gap_max'] = max(g for g, _ in gaps) / 60
    m['gap_from'] = min(t for _, t in gaps)
    m['gap_to'] = max(t + datetime.timedelta(seconds=g) for g, t in gaps)
    m['gaps_in_night'] = sum(1 for _, t in gaps if m['night'][0] <= t.hour < m['night'][1])
    return m


def _week_blocks(days):
    """Split the observed days into up to three labelled blocks of <=7 days."""
    blocks, i = [], 0
    while i < len(days):
        j = min(i + 6, len(days) - 1)
        if len(days) - j <= 3 and blocks:  # fold a short tail into the last block
            j = len(days) - 1
        blocks.append((i, j, (days[i], days[j])))
        i = j + 1
    return blocks


def _binom_tail(n, k, p):
    """P(X >= k) for X ~ Binomial(n, p) - is the quiet-hour surplus chance?"""
    return sum(math.comb(n, i) * p ** i * (1 - p) ** (n - i) for i in range(k, n + 1))


# --------------------------------------------------------------------------- #
# text
# --------------------------------------------------------------------------- #

TEXT = {
    'en': {
        'title': 'PokerTH Lobby: Player Growth',
        'subtitle': 'Server log {first} – {last} · {logins} logins · analysed {from_} to {to} ({edge})',
        'edge': 'edge days incomplete',
        'edge_all': 'all days included',
        'tile_active': 'Active accounts / day',
        'tile_active_sub': 'mean · trend {slope}/week (r={r})',
        'tile_new': 'New registrations',
        'tile_new_sub': 'in {days} days · {first} → {last} per day',
        'tile_ret': 'Return within 7 days',
        'tile_ret_sub': '{back} of {total} new accounts',
        'tile_base': 'Established base wk1 → wk{n}',
        'tile_base_sub': '{before} → {after} · {gone} gone, {back} returned',
        'a_title': 'Daily active registered accounts',
        'a_flat': 'flat – day-to-day noise (sd {sd}) is larger than the trend',
        'a_moving': '{dir} \u2013 r={r} against day-to-day noise (sd {sd})',
        'a_rising': 'rising', 'a_falling': 'falling',
        'a_trend': 'trend {slope}/day',
        'b_title': 'New accounts per day (growth of the highest dbId)',
        'b_rising': 'clearly rising – {ratio}x the first week',
        'b_falling': 'falling – {ratio}x the first week',
        'b_flat': 'roughly flat across the period',
        'b_avg': '{range}   avg {mean}',
        'c_title': 'Return rate of newly registered accounts',
        'c_rows': ['next day', 'within 3 days', 'within 7 days'],
        'c_note': 'Of {total} registrations only {seen} ever logged in ({pct}%).',
        'd_title': 'Active days of newcomers',
        'd_axis': 'active days within the observation window',
        'd_note1': '{pct}% of newcomers were active on exactly one day (avg {new} days;',
        'd_note2': 'established players: avg {old} days).',
        'e_title': 'Time of day: the gap between {from_} and {to} a.m.',
        'e_sub': 'The daily low in absolute terms – yet new accounts are clearly over-represented there',
        'e_sub_flat': 'The daily low in absolute terms – new accounts arrive there in proportion to traffic',
        'e_axis1': 'Logins per hour (daily mean)',
        'e_axis2': 'Share of new accounts relative to traffic (1.0 = proportional)',
        'e_axis3': 'Hour (server time)',
        'e_note1': '{win} holds just {logins}% of all logins, but {new}% of all first logins '
                   'by new accounts (index {index}, binomial test p = {p}).',
        'e_note2': 'All {n} longest login pauses in the period ({lo}–{hi} min) fall between {from_} and {to}.',
        'e_note2_mixed': '{k} of the {n} longest login pauses ({lo}–{hi} min) fall inside this window.',
        'f_title': 'Bottom line',
        'f1': 'Intake is growing ({from_} → {to} new accounts/day), the active base is not: '
              '{pct}% of the newcomers never come back,',
        'f1_flat': 'Intake is steady at about {to} new accounts/day and the active base is flat: '
                   '{pct}% of the newcomers never come back,',
        'f2': 'and in the established base {back} returners replace the {gone} who left. '
              'The lever is retention, not acquisition – above all',
        'f3': 'in the {win} window, where proportionally {mult}x as many newcomers arrive '
              'as there is traffic – and nobody to play with.',
        'f3_flat': 'because every cohort loses most of its arrivals within a day of signing up.',
        'tip_active': '{day}: {n} active accounts',
        'tip_new': '{day}: {n} new accounts',
        'tip_ret': '{back} of {total}',
        'tip_hist': '{n} accounts with {days} active days',
        'tip_hour': '{hour}:00 – {n} logins/day',
        'tip_index': '{hour}:00 – index {n}',
        'month': '%B %Y', 'day': '%d', 'daytip': '%d %b', 'range': '%d %b', 'stamp': '%d %b %Y',
        'decimal': '.',
    },
    'de': {
        'title': 'PokerTH-Lobby: Spielerentwicklung',
        'subtitle': 'Serverlog {first} – {last} · {logins} Logins · ausgewertet {from_} bis {to} ({edge})',
        'edge': 'Randtage unvollständig',
        'edge_all': 'alle Tage enthalten',
        'tile_active': 'Aktive Accounts / Tag',
        'tile_active_sub': 'Ø · Trend {slope}/Woche (r={r})',
        'tile_new': 'Neuregistrierungen',
        'tile_new_sub': 'in {days} Tagen · {first} → {last} pro Tag',
        'tile_ret': 'Wiederkehr nach 7 Tagen',
        'tile_ret_sub': '{back} von {total} neuen Accounts',
        'tile_base': 'Bestand Wo1 → Wo{n}',
        'tile_base_sub': '{before} → {after} · {gone} weg, {back} zurück',
        'a_title': 'Täglich aktive registrierte Accounts',
        'a_flat': 'flach – die Schwankung (sd {sd}) ist größer als der Trend',
        'a_moving': '{dir} \u2013 r={r} gegen die Schwankung (sd {sd})',
        'a_rising': 'steigend', 'a_falling': 'fallend',
        'a_trend': 'Trend {slope}/Tag',
        'b_title': 'Neue Accounts pro Tag (Zuwachs der höchsten dbId)',
        'b_rising': 'klar steigend – {ratio}-faches der ersten Woche',
        'b_falling': 'fallend – {ratio}-faches der ersten Woche',
        'b_flat': 'über den Zeitraum weitgehend konstant',
        'b_avg': '{range}   Ø {mean}',
        'c_title': 'Wiederkehr neu registrierter Accounts',
        'c_rows': ['nach 1 Tag', 'binnen 3 Tagen', 'binnen 7 Tagen'],
        'c_note': 'Von {total} Registrierungen haben sich nur {seen} je eingeloggt ({pct} %).',
        'd_title': 'Aktive Tage der Neuzugänge',
        'd_axis': 'aktive Tage im Beobachtungsfenster',
        'd_note1': '{pct} % der Neuzugänge waren an genau einem Tag aktiv (Ø {new} Tage;',
        'd_note2': 'Bestandsspieler: Ø {old} Tage).',
        'e_title': 'Tagesgang: die Lücke zwischen {from_} und {to} Uhr',
        'e_sub': 'Absolut das Tagesminimum – aber neue Accounts sind dort deutlich überrepräsentiert',
        'e_sub_flat': 'Absolut das Tagesminimum – neue Accounts kommen dort proportional zum Verkehr an',
        'e_axis1': 'Logins pro Stunde (Tagesmittel)',
        'e_axis2': 'Anteil an neuen Accounts relativ zum Verkehr (1,0 = proportional)',
        'e_axis3': 'Stunde (Serverzeit)',
        'e_note1': '{win}: nur {logins} % aller Logins, aber {new} % aller Erst-Logins neuer '
                   'Accounts (Index {index}, Binomialtest p = {p}).',
        'e_note2': 'Alle {n} längsten Login-Pausen des Zeitraums ({lo}–{hi} min) liegen zwischen {from_} und {to}.',
        'e_note2_mixed': '{k} der {n} längsten Login-Pausen ({lo}–{hi} min) liegen in diesem Fenster.',
        'f_title': 'Fazit',
        'f1': 'Der Zulauf wächst ({from_} → {to} neue Accounts/Tag), die aktive Basis nicht: '
              '{pct} % der Neuen kommen nie wieder,',
        'f1_flat': 'Der Zulauf liegt konstant bei rund {to} neuen Accounts/Tag, die aktive Basis '
                   'steht still: {pct} % der Neuen kommen nie wieder,',
        'f2': 'und im Bestand ersetzen {back} Rückkehrer die {gone} Abwanderer. '
              'Wachstumshebel ist Retention, nicht Akquise – besonders',
        'f3': 'im Fenster {win}, wo anteilig {mult}-mal so viele Neulinge ankommen '
              'wie Verkehr da ist, aber niemand zum Spielen.',
        'f3_flat': 'denn jede Kohorte verliert die meisten Zugänge schon am Tag nach der Anmeldung.',
        'tip_active': '{day}: {n} aktive Accounts',
        'tip_new': '{day}: {n} neue Accounts',
        'tip_ret': '{back} von {total}',
        'tip_hist': '{n} Accounts mit {days} aktiven Tagen',
        'tip_hour': '{hour} Uhr: {n} Logins/Tag',
        'tip_index': '{hour} Uhr: Index {n}',
        'month': '%B %Y', 'day': '%d.', 'daytip': '%d.%m.', 'range': '%d.%m.', 'stamp': '%d.%m.%Y',
        'decimal': ',',
    },
}

MONTH_NAMES = {
    'en': ['January', 'February', 'March', 'April', 'May', 'June',
           'July', 'August', 'September', 'October', 'November', 'December'],
    'de': ['Januar', 'Februar', 'März', 'April', 'Mai', 'Juni',
           'Juli', 'August', 'September', 'Oktober', 'November', 'Dezember'],
}
SHORT_MONTHS = {
    'en': ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'],
    'de': ['Jan', 'Feb', 'Mär', 'Apr', 'Mai', 'Jun', 'Jul', 'Aug', 'Sep', 'Okt', 'Nov', 'Dez'],
}


class Lang:
    """Localised strings plus the number and date formatting that goes with them."""

    def __init__(self, code):
        self.code = code
        self.s = TEXT[code]

    def __getitem__(self, key):
        return self.s[key]

    def num(self, value, digits=1, sign=False):
        text = ('%+.*f' if sign else '%.*f') % (digits, value)
        return text.replace('.', self.s['decimal'])

    def thousands(self, value):
        sep = '.' if self.code == 'de' else ','
        return '{:,}'.format(value).replace(',', sep)

    def date(self, day, fmt='range'):
        pattern = self.s[fmt]
        text = pattern.replace('%d', '%02d' % day.day)
        text = text.replace('%m', '%02d' % day.month)
        text = text.replace('%b', SHORT_MONTHS[self.code][day.month - 1])
        text = text.replace('%B', MONTH_NAMES[self.code][day.month - 1])
        return text.replace('%Y', str(day.year))

    def hour_range(self, night):
        return '%02d:00\u2013%02d:00' % night

    def clock(self, when):
        return when.strftime('%H:%M')


def nice_ceil(value):
    """Round up to a friendly axis maximum (1, 2, 2.5 or 5 times a power of ten)."""
    if value <= 0:
        return 1
    power = 10 ** math.floor(math.log10(value))
    for factor in (1, 2, 2.5, 5, 10):
        if value <= factor * power:
            return factor * power
    return 10 * power


def nice_axis(top, target=4, bottom=0):
    """Axis maximum and tick step that fit `top` snugly - no half-empty panel."""
    step = nice_ceil((top - bottom) / target)
    return math.ceil(top / step) * step, step


def axis_ticks(lo, hi, step):
    ticks, value = [], math.ceil(lo / step) * step
    while value <= hi + 1e-9:
        ticks.append(value)
        value += step
    return ticks


# --------------------------------------------------------------------------- #
# rendering
# --------------------------------------------------------------------------- #

CSS = """
  .bg{fill:#fcfcfb} .tile{fill:#f4f3f0;stroke:#e4e2dd}
  text{fill:#0b0b0b}
  .h1{font-size:26px;font-weight:700}
  .h2{font-size:15px;font-weight:600}
  .sub,.note,.axis,.tilekey,.tilesub{fill:#52514e}
  .sub{font-size:13px} .note{font-size:11.5px} .axis{font-size:11px}
  .tilekey{font-size:11.5px;font-weight:600;letter-spacing:.03em;text-transform:uppercase}
  .tileval{font-size:30px;font-weight:700} .tilesub{font-size:11px}
  .body{font-size:13px}
  .val{font-size:11.5px;font-weight:600;paint-order:stroke fill;stroke:#fcfcfb;
       stroke-width:3;stroke-linejoin:round}
  .grid{stroke:#e4e2dd;stroke-width:1}
  .line{fill:none;stroke-width:2}
  .trend{fill:none;stroke:#8a8880;stroke-width:1.5;stroke-dasharray:5 4}
  .mean{stroke:#0b0b0b;stroke-width:2}
  .meanlab{font-size:11.5px;font-weight:600;fill:#0b0b0b}
  .track{fill:#e4e2dd}
  .band{fill:#eb6834;opacity:.09}
  .bandlab{font-size:11px;font-weight:600;fill:#c14a1f}
  .mutedf{fill:#b9b7b0}
  .dot{stroke:#fcfcfb;stroke-width:2}
  .bar{stroke:#fcfcfb;stroke-width:2}
  .s1{stroke:#2a78d6} .s1f{fill:#2a78d6} .s1t{fill:#2a78d6}
  .s2f{fill:#eb6834} .s2t{fill:#c14a1f}
  .tone-up{fill:#008300} .tone-down{fill:#c14a1f}
  @media (prefers-color-scheme: dark) {
    .bg{fill:#1a1a19} .tile{fill:#232321;stroke:#3a3936}
    text{fill:#ffffff}
    .sub,.note,.axis,.tilekey,.tilesub{fill:#c3c2b7}
    .grid{stroke:#3a3936} .track{fill:#3a3936}
    .band{fill:#d95926;opacity:.16} .bandlab{fill:#e8794a} .mutedf{fill:#5c5a54}
    .trend{stroke:#9a988f} .mean{stroke:#ffffff} .meanlab{fill:#ffffff}
    .val{stroke:#1a1a19}
    .dot,.bar{stroke:#1a1a19}
    .s1{stroke:#3987e5} .s1f{fill:#3987e5} .s1t{fill:#3987e5}
    .s2f{fill:#d95926} .s2t{fill:#e8794a}
    .tone-up{fill:#3fb950} .tone-down{fill:#e8794a}
  }
"""

W, H = 1280, 1530


class Canvas:
    def __init__(self):
        self.parts = []

    def text(self, x, y, s, cls='', anchor='start'):
        self.parts.append('<text x="%.1f" y="%.1f" class="%s" text-anchor="%s">%s</text>'
                          % (x, y, cls, anchor, html.escape(str(s))))

    def rect(self, x, y, w, h, cls, rx=0, tip=None):
        body = '<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%d" class="%s"' % (
            x, y, max(w, 0), max(h, 0), rx, cls)
        if tip:
            self.parts.append(body + '><title>%s</title></rect>' % html.escape(tip))
        else:
            self.parts.append(body + '/>')

    def line(self, x1, y1, x2, y2, cls):
        self.parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" class="%s"/>'
                          % (x1, y1, x2, y2, cls))

    def poly(self, points, cls):
        self.parts.append('<polyline class="%s" points="%s"/>'
                          % (cls, ' '.join('%.1f,%.1f' % p for p in points)))

    def dot(self, x, y, r, cls, tip):
        self.parts.append('<circle cx="%.1f" cy="%.1f" r="%.1f" class="%s"><title>%s</title></circle>'
                          % (x, y, r, cls, html.escape(tip)))

    def svg(self):
        return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
                'font-family="Inter, Segoe UI, Helvetica, Arial, sans-serif">\n'
                '<style>%s</style>\n<rect class="bg" x="0" y="0" width="%d" height="%d"/>\n%s\n</svg>\n'
                % (W, H, W, H, CSS, W, H, '\n'.join(self.parts)))


def render(m, L):
    c = Canvas()
    days, active, newreg = m['days'], m['active_per_day'], m['new_per_day']
    labels = [L.date(d, 'day') for d in days]

    # ---- header
    c.text(48, 58, L['title'], 'h1')
    c.text(48, 88, L['subtitle'].format(
        first=L.date(m['log_first'], 'range'), last=L.date(m['log_last'], 'stamp'),
        logins=L.thousands(m['total_logins']),
        from_=L.date(days[0], 'range'), to=L.date(days[-1], 'range'),
        edge=L['edge'] if days[0] != m['log_first'] else L['edge_all']), 'sub')

    # ---- headline tiles
    back7, total7 = m['retention'][7]
    tiles = [
        (L['tile_active'], '%.0f' % m['active_mean'],
         L['tile_active_sub'].format(slope=L.num(m['slope'] * 7, 1, sign=True), r=L.num(m['r'], 2)),
         'flat'),
        (L['tile_new'], str(m['new_total']),
         L['tile_new_sub'].format(days=m['new_days'], first=L.num(m['week_means'][0]),
                                  last=L.num(m['week_means'][-1])),
         'up' if m['new_ratio'] > 1.15 else 'down' if m['new_ratio'] < 0.85 else 'flat'),
        (L['tile_ret'], '%.0f%%' % (100 * back7 / total7) if L.code == 'en'
         else '%.0f %%' % (100 * back7 / total7),
         L['tile_ret_sub'].format(back=back7, total=total7),
         'up' if back7 / total7 >= .5 else 'down'),
        (L['tile_base'].format(n=len(m['weeks'])), '%+d' % (m['base_after'] - m['base_before']),
         L['tile_base_sub'].format(before=m['base_before'], after=m['base_after'],
                                   gone=m['base_gone'], back=m['base_back']),
         'up' if m['base_after'] > m['base_before'] else
         'down' if m['base_after'] < m['base_before'] else 'flat'),
    ]
    tw = (W - 96 - 3 * 20) / 4
    for i, (key, value, sub, tone) in enumerate(tiles):
        tx = 48 + i * (tw + 20)
        c.rect(tx, 112, tw, 104, 'tile', rx=10)
        c.text(tx + 18, 138, key, 'tilekey')
        c.text(tx + 18, 180, value, 'tileval tone-%s' % tone)
        c.text(tx + 18, 202, sub, 'tilesub')

    # ---- panel A: daily active accounts
    AX, AY, AW, AH = 48, 268, W - 96, 196
    c.text(AX, AY - 24, L['a_title'], 'h2')
    # A weak correlation means the day-to-day noise dominates whatever drift
    # the regression found - calling that a trend would overstate it.
    if abs(m['r']) < 0.5:
        note = L['a_flat'].format(sd=L.num(m['active_sd'], 0))
    else:
        note = L['a_moving'].format(
            dir=L['a_rising'] if m['slope'] > 0 else L['a_falling'],
            r=L.num(abs(m['r']), 2), sd=L.num(m['active_sd'], 0))
    c.text(AX + AW, AY - 24, note, 'note', 'end')

    pad = max(5, (max(active) - min(active)) * .35)
    step = nice_ceil((max(active) - min(active) + 2 * pad) / 5)
    lo = math.floor((min(active) - pad) / step) * step
    hi = math.ceil((max(active) + pad) / step) * step
    ya = lambda v: AY + AH - (v - lo) / (hi - lo) * AH
    for tick in axis_ticks(lo, hi, step):
        c.line(AX, ya(tick), AX + AW, ya(tick), 'grid')
        c.text(AX - 10, ya(tick) + 4, '%g' % tick, 'axis', 'end')
    step = AW / (len(active) - 1)
    xa = lambda i: AX + i * step
    mx = statistics.mean(range(len(active)))
    c.poly([(xa(i), ya(m['active_mean'] + m['slope'] * (i - mx))) for i in (0, len(active) - 1)],
           'trend')
    c.poly([(xa(i), ya(v)) for i, v in enumerate(active)], 'line s1')
    for i, v in enumerate(active):
        c.dot(xa(i), ya(v), 4.5, 'dot s1f',
              L['tip_active'].format(day=L.date(days[i], 'daytip'), n=v))
    c.text(xa(0), ya(active[0]) - 14, active[0], 'val s1t')
    c.text(xa(len(active) - 1), ya(active[-1]) - 14, active[-1], 'val s1t', 'middle')
    c.text(xa(len(active) - 1), ya(m['active_mean'] + m['slope'] * (len(active) - 1 - mx)) + 20,
           L['a_trend'].format(slope=L.num(m['slope'], 1, sign=True)), 'note s1t', 'end')
    for i, label in enumerate(labels):
        c.text(xa(i), AY + AH + 22, label, 'axis', 'middle')
    c.text(AX + AW / 2, AY + AH + 42, L.date(days[len(days) // 2], 'month'), 'axis', 'middle')

    # ---- panel B: new registrations per day
    BX, BY, BW, BH = 48, 540, W - 96, 180
    c.text(BX, BY - 24, L['b_title'], 'h2')
    if m['new_ratio'] > 1.3:
        note = L['b_rising'].format(ratio=L.num(m['new_ratio'], 1))
    elif m['new_ratio'] < 0.77:
        note = L['b_falling'].format(ratio=L.num(m['new_ratio'], 1))
    else:
        note = L['b_flat']
    c.text(BX + BW, BY - 24, note, 'note', 'end')

    bhi, bstep = nice_axis(max(v for v in newreg if v is not None), 3)
    yb = lambda v: BY + BH - v / bhi * BH
    for tick in axis_ticks(0, bhi, bstep):
        c.line(BX, yb(tick), BX + BW, yb(tick), 'grid')
        c.text(BX - 10, yb(tick) + 4, '%g' % tick, 'axis', 'end')
    bw = BW / len(newreg)
    for i, v in enumerate(newreg):
        if v is None:
            continue  # no predecessor day, so no delta to draw
        bx = BX + i * bw + 2
        c.rect(bx, yb(v), bw - 4, BY + BH - yb(v), 'bar s2f', rx=4,
               tip=L['tip_new'].format(day=L.date(days[i], 'daytip'), n=v))
        c.text(bx + (bw - 4) / 2, yb(v) - 8, v, 'val s2t', 'middle')
    for (a, b, (d0, d1)), mean in zip(m['weeks'], m['week_means']):
        while newreg[a] is None and a < b:
            a += 1  # do not draw the mean across a slot that has no value
        x1, x2 = BX + a * bw + 2, BX + (b + 1) * bw - 2
        c.line(x1, yb(mean), x2, yb(mean), 'mean')
        c.text((x1 + x2) / 2, BY + BH + 40, L['b_avg'].format(
            range='%s-%s' % (L.date(d0, 'range'), L.date(d1, 'range')),
            mean=L.num(mean)), 'meanlab', 'middle')
    for i, label in enumerate(labels):
        c.text(BX + i * bw + bw / 2, BY + BH + 20, label, 'axis', 'middle')

    # ---- panel C: return rate
    CX, CY, CW, CH = 48, 810, 560, 150
    c.text(CX, CY - 24, L['c_title'], 'h2')
    for i, (label, window) in enumerate(zip(L['c_rows'], (1, 3, 7))):
        back, total = m['retention'][window]
        y = CY + i * 44
        c.text(CX, y + 18, label, 'axis')
        c.rect(CX + 140, y + 4, CW - 260, 20, 'track', rx=4)
        c.rect(CX + 140, y + 4, (CW - 260) * back / total, 20, 's2f', rx=4,
               tip=L['tip_ret'].format(back=back, total=total))
        pct = '%.0f%%' % (100 * back / total) if L.code == 'en' else '%.0f %%' % (100 * back / total)
        c.text(CX + CW - 108, y + 19, pct, 'val s2t')
        c.text(CX + CW - 62, y + 19, '(%d/%d)' % (back, total), 'axis')
    c.text(CX, CY + CH - 6, L['c_note'].format(
        total=m['new_total'], seen=m['new_seen'],
        pct='%.0f' % (100 * m['new_seen'] / m['new_total'])), 'note')

    # ---- panel D: how long newcomers stay
    DX, DY, DW, DH = 676, 810, W - 48 - 676, 116
    c.text(DX, DY - 24, L['d_title'], 'h2')
    top = max(m['hist'].values())
    mw = DW / HIST_MAX
    for i in range(1, HIST_MAX + 1):
        v = m['hist'].get(i, 0)
        h = v / top * DH
        bx = DX + (i - 1) * mw + 3
        bucket = '%d%s' % (i, '+' if i == HIST_MAX else '')
        c.rect(bx, DY + DH - h, mw - 6, h, 'bar s2f', rx=4,
               tip=L['tip_hist'].format(n=v, days=bucket))
        if v:
            c.text(bx + (mw - 6) / 2, DY + DH - h - 8, v, 'val s2t', 'middle')
        c.text(bx + (mw - 6) / 2, DY + DH + 20, bucket, 'axis', 'middle')
    c.text(DX + DW / 2, DY + DH + 40, L['d_axis'], 'axis', 'middle')
    c.text(DX, DY + DH + 64, L['d_note1'].format(
        pct='%.0f' % (100 * m['one_day_share']), new=L.num(m['newcomer_days'])), 'note')
    c.text(DX, DY + DH + 82, L['d_note2'].format(old=L.num(m['established_days'])), 'note')

    # ---- panel E: time of day
    EX, EY, EW, EH = 48, 1042, W - 96, 120
    night = m['night']
    win = L.hour_range(night)
    c.text(EX, EY - 52, L['e_title'].format(from_='%d' % night[0], to='%d' % night[1]), 'h2')
    c.text(EX, EY - 30, L['e_sub'] if m['night_index'] >= 1.25 else L['e_sub_flat'], 'note')
    xh = lambda h: EX + h * (EW / 24)
    nbx, nbw = xh(night[0]), (night[1] - night[0]) * (EW / 24)
    c.rect(nbx, EY - 8, nbw, EH + 8, 'band', rx=6)
    ehi, estep = nice_axis(max(m['logins_per_hour']), 3)
    ye = lambda v: EY + EH - v / ehi * EH
    for tick in axis_ticks(0, ehi, estep):
        c.line(EX, ye(tick), EX + EW, ye(tick), 'grid')
        c.text(EX - 10, ye(tick) + 4, '%g' % tick, 'axis', 'end')
    hw = EW / 24
    for h in range(24):
        v = m['logins_per_hour'][h]
        c.rect(xh(h) + 2.5, ye(v), hw - 5, EY + EH - ye(v), 'bar s1f', rx=4,
               tip=L['tip_hour'].format(hour='%02d' % h, n=L.num(v)))
    c.text(EX + 4, ye(ehi * 0.97) + 4, L['e_axis1'], 'axis')
    c.text(nbx + nbw / 2, EY - 18, win, 'bandlab', 'middle')

    FX, FY, FW, FH = EX, EY + EH + 58, EW, 86
    c.text(FX, FY - 14, L['e_axis2'], 'axis')
    c.rect(nbx, FY - 6, nbw, FH + 6, 'band', rx=6)
    ihi = max(2.0, nice_axis(max(m['hour_index']), 2)[0])
    yf = lambda v: FY + FH - v / ihi * FH
    c.line(FX, yf(1), FX + FW, yf(1), 'trend')
    c.text(FX - 10, yf(1) + 4, L.num(1, 1), 'axis', 'end')
    for h in range(24):
        idx = m['hour_index'][h]
        c.rect(xh(h) + 2.5, yf(min(idx, ihi)), hw - 5, FY + FH - yf(min(idx, ihi)),
               'bar %s' % ('s2f' if idx >= 1 else 'mutedf'), rx=4,
               tip=L['tip_index'].format(hour='%02d' % h, n=L.num(idx, 2)))
        c.text(xh(h) + 2.5 + (hw - 5) / 2, FY + FH + 18, '%02d' % h, 'axis', 'middle')
    c.text(FX + FW / 2, FY + FH + 36, L['e_axis3'], 'axis', 'middle')
    c.text(FX, FY + FH + 62, L['e_note1'].format(
        win=win, logins=L.num(100 * m['night_login_share']), new=L.num(100 * m['night_new_share']),
        index=L.num(m['night_index'], 2), p=L.num(m['night_p'], 3)), 'note')
    if m['gaps_in_night'] == m['gap_count']:
        gap_note = L['e_note2'].format(n=m['gap_count'], lo='%.0f' % m['gap_min'],
                                       hi='%.0f' % m['gap_max'],
                                       from_=L.clock(m['gap_from']), to=L.clock(m['gap_to']))
    else:
        gap_note = L['e_note2_mixed'].format(k=m['gaps_in_night'], n=m['gap_count'],
                                             lo='%.0f' % m['gap_min'], hi='%.0f' % m['gap_max'])
    c.text(FX, FY + FH + 80, gap_note, 'note')

    # ---- bottom line
    GY = 1410
    c.rect(48, GY, W - 96, 100, 'tile', rx=10)
    c.text(68, GY + 28, L['f_title'], 'h2')
    never = 100 * (1 - m['retention'][7][0] / m['retention'][7][1])
    key = 'f1' if m['new_ratio'] > 1.15 else 'f1_flat'
    c.text(68, GY + 52, L[key].format(from_=L.num(m['week_means'][0], 0),
                                      to=L.num(m['week_means'][-1], 0),
                                      pct='%.0f' % never), 'body')
    c.text(68, GY + 72, L['f2'].format(back=m['base_back'], gone=m['base_gone']), 'body')
    if m['night_index'] >= 1.25:
        c.text(68, GY + 92, L['f3'].format(win=win, mult=L.num(m['night_index'], 1)), 'body')
    else:
        c.text(68, GY + 92, L['f3_flat'], 'body')
    return c.svg()


# --------------------------------------------------------------------------- #

def summary(m, L):
    back7, total7 = m['retention'][7]
    out = [
        '%s .. %s, %d logins, %d full days analysed'
        % (m['log_first'], m['log_last'], m['total_logins'], len(m['days'])),
        'active registered accounts/day: mean %.1f, sd %.1f, trend %+.2f/day (r=%.2f)'
        % (m['active_mean'], m['active_sd'], m['slope'], m['r']),
        'new registrations: %d over %d days, %s per day by week'
        % (m['new_total'], m['new_days'], ' -> '.join('%.1f' % v for v in m['week_means'])),
        'return rate: %s' % ', '.join(
            'D+%d %.0f%% (%d/%d)' % (w, 100 * b / t, b, t) for w, (b, t) in m['retention'].items()),
        'newcomers active on %.1f days vs %.1f for established players; %.0f%% on a single day'
        % (m['newcomer_days'], m['established_days'], 100 * m['one_day_share']),
        'established base %d -> %d (%d gone, %d returned)'
        % (m['base_before'], m['base_after'], m['base_gone'], m['base_back']),
        'quiet window %02d:00-%02d:00: %.1f%% of logins, %.1f%% of first logins, index %.2f, p=%.4f'
        % (m['night'][0], m['night'][1], 100 * m['night_login_share'],
           100 * m['night_new_share'], m['night_index'], m['night_p']),
    ]
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('log', help='pokerth_server message log')
    ap.add_argument('--lang', choices=sorted(TEXT), default='en', help='report language')
    ap.add_argument('-o', '--out', help='output SVG (default: derived from --lang)')
    ap.add_argument('--keep-edges', action='store_true',
                    help='include the first and last day, which are usually partial')
    ap.add_argument('--quiet-window', metavar='FROM-TO',
                    help='pin the highlighted window, e.g. 5-9 (default: the quietest hours)')
    ap.add_argument('--quiet', action='store_true', help='do not print the text summary')
    args = ap.parse_args()

    L = Lang(args.lang)
    logins, stamps = parse(args.log)
    night = None
    if args.quiet_window:
        try:
            a, b = (int(v) for v in args.quiet_window.split('-'))
        except ValueError:
            ap.error('--quiet-window expects FROM-TO, e.g. 5-9')
        if not 0 <= a < b <= 24:
            ap.error('--quiet-window must lie inside 0-24')
        night = (a, b)
    m = analyse(logins, stamps, args.keep_edges, night)
    out = args.out or ('player-trend-analysis.svg' if args.lang == 'en'
                       else 'spieler-trend-analyse.svg')
    with open(out, 'w', encoding='utf-8') as fh:
        fh.write(render(m, L))
    if not args.quiet:
        print(summary(m, L))
    print('wrote %s' % out)


if __name__ == '__main__':
    main()
