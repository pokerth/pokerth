# QMake pro-file for PokerTH

!c++11 {
	system( $$QMAKE_CXX -dumpversion | grep -e "^6.[0-9]" > /dev/null ) {
		greaterThan(QT_MAJOR_VERSION, 5) | equals(QT_MAJOR_VERSION, 5) {
			CONFIG += c++11 
		}
		else { 
			equals(QT_MAJOR_VERSION, 4) : greaterThan(QT_MINOR_VERSION, 7) {
				CONFIG += "c++11" 
				QMAKE_CXXFLAGS += "-std=gnu++11"
			}
			else {
				error (QT must be greater and 4.8+) 
			}
		}
	}
}

c++11 {
	!system( $$QMAKE_CXX -dumpversion | grep -e "^6.[0-9]" > /dev/null ) {
		equals(QT_MAJOR_VERSION, 4) : greaterThan(QT_MINOR_VERSION, 7) {
			QMAKE_CXXFLAGS += "-std=gnu++11"
		}
	}
}
