;;; 
;;; Copyright (C) 2007 Lothar May l-may@gmx.de
;;; 
;;; All rights reserved.
;;; 
;;; Redistribution and use in source and binary forms, with or
;;; without modification, are permitted provided that the
;;; following conditions are met:
;;; 1. Redistributions of source code must retain the above
;;;    copyright notice, this list of conditions and the
;;;    following disclaimer.
;;; 2. Redistributions in binary form must reproduce the
;;;    above copyright notice, this list of conditions and
;;;    the following disclaimer in the documentation and/or
;;;    other materials provided with the distribution.
;;; 3. Neither the name of the project nor the names of
;;;    its contributors may be used to endorse or promote
;;;    products derived from this software without specific
;;;    prior written permission.
;;;  
;;; THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS
;;; ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
;;; BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
;;; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;;; DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS
;;; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;;; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;;; LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
;;; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;;; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
;;; IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;;; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
;;; USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
;;; OF SUCH DAMAGE.

;;; PKTH functions
(load "pkth.scm")

(define pkth-test-list '())

(define (pkth-test-init-packet-too-large)
  (let ((sock (pkth-connect)))
    (display "Sending loads of large init packets...\n")
    (dotimes (n 1024)
             (pkth-send-message-nolog
              sock
              (pkth-create-packet
               pkth-type-init
               (list
                (uint16->bytes pkth-version-major)
                (uint16->bytes pkth-version-minor)
                (uint16->bytes (string-length ""))
                (uint16->bytes (string-length "client1"))
                (uint16->bytes 0)
                (uint16->bytes 0)
                (append-padding (string->bytes ""))
                (append-padding (string->bytes "client1"))
                (make-list 1024 0)))))
    (sock-close sock)
    ))
(set! pkth-test-list (test-register pkth-test-list "PKTH: Init with too large packets" pkth-test-init-packet-too-large))

(define (pkth-test-init)
  (dotimes (n 1024)
           (let ((sock (pkth-connect)))
             (pkth-send-message sock (pkth-create-init '() "" (number->string n)))
             (display "Waiting for Init-Ack...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-init-ack?) '() 5000)
             (sock-close sock))))

(set! pkth-test-list (test-register pkth-test-list "PKTH: Init" pkth-test-init))

(define (pkth-test-create-destroy-game)
  (let ((sock (pkth-connect)))
    (pkth-send-message sock (pkth-create-init '() "" "client1"))
    (display "Waiting for Init-Ack...\n")
    (wait-for-message sock pkth-recv-message (list pkth-is-type-init-ack?) '() 5000)
    (display "Creating and destroying loads of games...\n")
    (dotimes (n 256)
             (pkth-send-message sock
                                (pkth-create-create-game
                                 (pkth-create-game-info
                                  7
                                  pkth-raise-interval-mode-on-hand
                                  4
                                  pkth-raise-mode-double-blinds
                                  pkth-end-raise-mode-double-blinds
                                  11
                                  20 ; player action timeout
                                  40 ; first small blind
                                  0 ; end raise small blind
                                  2000 ; start money
                                  '() ; manual blinds
                                  )
                                 "test game"
                                 "test password"
                                 ))
             (display "Waiting for Game List New / Join Game Ack / Game List Player Joined...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed?) 5000)
             (pkth-send-message sock (pkth-create-leave-current-game))
             (display "Waiting for Game List Player Left...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-player-left?) (list pkth-is-type-statistics-changed?) 5000)
             (display "Waiting for Removed From Game...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-removed-from-game?) (list pkth-is-type-statistics-changed?) 5000)
             (display "Waiting for Game List Update...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-update?) (list pkth-is-type-statistics-changed?) 5000)
             )
    (sock-close sock)
    ))
(set! pkth-test-list (test-register pkth-test-list "PKTH Test 2: Creating and destroying games" pkth-test-create-destroy-game))

(define (pkth-test-start-leave-game)
  (let ((sock (pkth-connect)))
    (pkth-send-message sock (pkth-create-init '() "" "client1"))
    (display "Waiting for Init-Ack...\n")
    (wait-for-message sock pkth-recv-message (list pkth-is-type-init-ack?) '() 5000)
    (display "Starting and leaving loads of games...\n")
    (dotimes (n 256)
             (pkth-send-message sock
                                (pkth-create-create-game
                                 (pkth-create-game-info
                                  7
                                  pkth-raise-interval-mode-on-hand
                                  4
                                  pkth-raise-mode-double-blinds
                                  pkth-end-raise-mode-double-blinds
                                  11
                                  20 ; player action timeout
                                  40 ; first small blind
                                  0 ; end raise small blind
                                  2000 ; start money
                                  '() ; manual blinds
                                  )
                                 "test game"
                                 "test password"
                                 ))
             (display "Waiting for Game List New / Join Game Ack / Game List Player Joined...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (pkth-send-message sock (pkth-create-start-event pkth-start-flag-fill-with-cpu-players))
             (display "Waiting for Start Event...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-start-event?) (list pkth-is-type-statistics-changed? pkth-is-type-player-joined? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update?) 5000)
             (pkth-send-message sock (pkth-create-start-event-ack))
             (display "Waiting for Game Start...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-start?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update?) 5000)
             (display "Waiting for Hand Start...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-hand-start?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update?) 5000)
             (display "Waiting for Players Turn...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-players-turn?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-players-action-done? pkth-is-type-game-list-update?) 5000)
             (pkth-send-message sock (pkth-create-leave-current-game))
             (display "Waiting for Removed From Game...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-removed-from-game?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             )
    (sock-close sock)
    ))
(set! pkth-test-list (test-register pkth-test-list "PKTH Test 3: Starting and leaving games" pkth-test-start-leave-game))

(define (pkth-test-run-game)
  (let ((sock (pkth-connect)))
    (pkth-send-message sock (pkth-create-init '() "" "client1"))
    (display "Waiting for Init-Ack...\n")
    (wait-for-message sock pkth-recv-message (list pkth-is-type-init-ack?) '() 5000)
    (display "Starting and leaving loads of games...\n")
    (dotimes (n 1)
             (pkth-send-message sock
                                (pkth-create-create-game
                                 (pkth-create-game-info
                                  7
                                  pkth-raise-interval-mode-on-hand
                                  4
                                  pkth-raise-mode-double-blinds
                                  pkth-end-raise-mode-double-blinds
                                  11
                                  20 ; player action timeout
                                  40 ; first small blind
                                  0 ; end raise small blind
                                  2000 ; start money
                                  '() ; manual blinds
                                  )
                                 "test game"
                                 "test password"
                                 ))
             (display "Waiting for Game List New / Join Game Ack / Game List Player Joined...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-list-new? pkth-is-type-join-game-ack? pkth-is-type-game-list-player-joined?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             (pkth-send-message sock (pkth-create-start-event pkth-start-flag-fill-with-cpu-players))
             (display "Waiting for Start Event...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-start-event?) (list pkth-is-type-statistics-changed? pkth-is-type-player-joined? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update?) 5000)
             (pkth-send-message sock (pkth-create-start-event-ack))
             (display "Waiting for Game Start...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-game-start?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update?) 5000)
             (display "Waiting for End Of Game...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-end-of-game?) (list pkth-is-valid-type?) 5000000)
             (pkth-send-message sock (pkth-create-leave-current-game))
             (display "Waiting for Removed From Game...\n")
             (wait-for-message sock pkth-recv-message (list pkth-is-type-removed-from-game?) (list pkth-is-type-statistics-changed? pkth-is-type-game-list-player-joined? pkth-is-type-game-list-update? pkth-is-type-game-list-player-left?) 5000)
             )
    (sock-close sock)
    ))
;(set! pkth-test-list (test-register pkth-test-list "PKTH Test 4: Running games" pkth-test-run-game))

(test-run-all pkth-test-list)

