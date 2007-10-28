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

;;; Version 1.0.0

;;; Socket functions
(load "sock.scm")
;;; Helper functions
(load "helper.scm")
;;; Network configuration
(load "pkth_config.scm")

;;; SCTP Payload Protocol Identifier for PKTH
(define pkth-ppid                               0)

;;; TCP/SCTP Port for PKTH
(define pkth-port                               7234)

;;; PKTH protocol version
(define pkth-version-major                      2)
(define pkth-version-minor                      0)

;;; PokerTH game version
(define pkth-game-version                       #x0006)
(define pkth-beta-revision                      #x0000)

(define pkth-type-init                          #x0001)
(define pkth-type-init-ack                      #x0002)
(define pkth-type-retrieve-avatar               #x0003)
(define pkth-type-avatar-header                 #x0004)
(define pkth-type-avatar-file                   #x0005)
(define pkth-type-avatar-end                    #x0006)
(define pkth-type-unknown-avatar                #x0007)
(define pkth-type-game-list-new                 #x0010)
(define pkth-type-game-list-update              #x0011)
(define pkth-type-game-list-player-joined       #x0012)
(define pkth-type-game-list-player-left         #x0013)
(define pkth-type-game-list-admin-changed       #x0014)
(define pkth-type-retrieve-player-info          #x0020)
(define pkth-type-player-info                   #x0021)
(define pkth-type-unknown-player-id             #x0022)
(define pkth-type-create-game                   #x0030)
(define pkth-type-join-game                     #x0031)
(define pkth-type-join-game-ack                 #x0032)
(define pkth-type-join-game-failed              #x0033)
(define pkth-type-player-joined                 #x0034)
(define pkth-type-player-left                   #x0035)
(define pkth-type-game-admin-changed            #x0036)
(define pkth-type-kick-player                   #x0040)
(define pkth-type-leave-current-game            #x0041)
(define pkth-type-start-event                   #x0042)
(define pkth-type-start-event-ack               #x0043)
(define pkth-type-game-start                    #x0050)
(define pkth-type-hand-start                    #x0051)
(define pkth-type-players-turn                  #x0052)
(define pkth-type-players-action                #x0053)
(define pkth-type-players-action-done           #x0054)
(define pkth-type-players-action-rejected       #x0055)
(define pkth-type-deal-flop-cards               #x0060)
(define pkth-type-deal-turn-card                #x0061)
(define pkth-type-deal-river-card               #x0062)
(define pkth-type-all-in-show-cards             #x0063)
(define pkth-type-end-of-hand-show-cards        #x0064)
(define pkth-type-end-of-hand-hide-cards        #x0065)
(define pkth-type-end-of-game                   #x0070)
(define pkth-type-statistics-changed            #x0080)

(define pkth-type-removed-from-game             #x0100)

(define pkth-type-send-chat-text                #x0200)
(define pkth-type-chat-text                     #x0201)

(define pkth-type-error                         #x0400)

(define pkth-game-flag-password-protected       #x01)

(define pkth-player-flag-human                  #x01)
(define pkth-player-flag-has-avatar             #x02)

(define pkth-start-flag-fill-with-cpu-players   #x01)

(define pkth-privacy-flag-show-avatar           #x01)

;;; Reasons why join game failed.
(define pkth-join-failed-game-full              #x0001)
(define pkth-join-failed-game-already-running   #x0002)
(define pkth-join-failed-invalid-password       #x0003)
(define pkth-join-failed-other-reason           #xFFFF)

;;; Reasons for being removed from a game.
(define pkth-removed-on-request                 #x0000)
(define pkth-removed-game-full                  #x0001)
(define pkth-removed-game-already-running       #x0002)
(define pkth-removed-kicked                     #x0003)
(define pkth-removed-other-reason               #xFFFF)

;;; Internal error codes.
(define pkth-err-reserved                       #x0000)
(define pkth-err-init-version-not-supported     #x0001)
(define pkth-err-init-server-full               #x0002)
(define pkth-err-init-invalid-password          #x0004)
(define pkth-err-init-player-name-in-use        #x0005)
(define pkth-err-init-invalid-player-name       #x0006)
(define pkth-err-init-server-maintenance        #x0007)
(define pkth-err-avatar-too-large               #x0010)
(define pkth-err-avatar-wrong-size              #x0011)
(define pkth-err-join-game-unknown-game         #x0020)
(define pkth-err-general-invalid-packet         #xFF01)
(define pkth-err-general-invalid-state          #xFF02)
(define pkth-err-general-player-kicked          #xFF03)
(define pkth-err-other                          #xFFFF)

;;; Constant for reserved
(define pkth-reserved                           0)

;;; Header lengths
(define pkth-header-length-common               4)

;;; Common header value lengths
(define pkth-length-type                        2)
(define pkth-length-msg-length                  2)

;;; Value lengths
(define pkth-length-version                     2)
(define pkth-length-revision                    2)
(define pkth-length-string-length               2)
(define pkth-length-flags                       2)
(define pkth-length-reserved                    2)
(define pkth-length-player-id                   4)
(define pkth-length-session-id                  4)
(define pkth-length-md5                         16)
(define pkth-length-blind-value                 2)

;;; Value offsets common header
(define pkth-offset-type                        0)
(define pkth-offset-msg-length                  (+ pkth-offset-type pkth-length-type))
(define pkth-offset-data                        (+ pkth-offset-msg-length pkth-length-msg-length))

;;; Value offsets pkth-type-init
(define pkth-init-offset-version-major          pkth-offset-data)
(define pkth-init-offset-version-minor          (+ pkth-init-offset-version-major pkth-length-version))
(define pkth-init-offset-password-length        (+ pkth-init-offset-version-minor pkth-length-version))
(define pkth-init-offset-player-name-length     (+ pkth-init-offset-password-length pkth-length-string-length))
(define pkth-init-offset-privacy-flags          (+ pkth-init-offset-player-name-length pkth-length-string-length))
(define pkth-init-offset-reserved               (+ pkth-init-offset-privacy-flags pkth-length-flags))
(define pkth-init-offset-avatar-md5             (+ pkth-init-offset-reserved pkth-length-reserved))
(define pkth-init-offset-password               (+ pkth-init-offset-avatar-md5 pkth-length-md5))

;;; Value offsets pkth-type-init-ack
(define pkth-init-ack-offset-game-version       pkth-offset-data)
(define pkth-init-ack-offset-beta-revision      (+ pkth-init-ack-offset-game-version pkth-length-version))
(define pkth-init-ack-offset-session-id         (+ pkth-init-ack-offset-beta-revision pkth-length-revision))
(define pkth-init-ack-offset-player-id          (+ pkth-init-ack-offset-session-id pkth-length-session-id))

;;; Minimum/maximum packet length
(define pkth-minimum-message-length             8)
(define pkth-maximum-message-length             268)

;;; Receive buf length
(define pkth-buf-length                         #xffff)

;;; Game info constants
(define pkth-raise-interval-mode-on-hand        1)
(define pkth-raise-interval-mode-on-minute      2)

(define pkth-raise-mode-double-blinds           1)
(define pkth-raise-mode-manual-blinds-order     2)

(define pkth-end-raise-mode-double-blinds       1)
(define pkth-end-raise-mode-raise               2)
(define pkth-end-raise-mode-keep-last-blind     3)

;;;
;;; Header constructors
;;;

(define pkth-create-packet
  (lambda (type data)
    (append
     (uint16->bytes type)
     (uint16->bytes (+ pkth-header-length-common (apply + (map length data))))
     (apply append data))))

(define pkth-create-md5
  (lambda (m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 mA mB mC mD mE mF)
    (append
     (uint8->bytes m0)
     (uint8->bytes m1)
     (uint8->bytes m2)
     (uint8->bytes m3)
     (uint8->bytes m4)
     (uint8->bytes m5)
     (uint8->bytes m6)
     (uint8->bytes m7)
     (uint8->bytes m8)
     (uint8->bytes m9)
     (uint8->bytes mA)
     (uint8->bytes mB)
     (uint8->bytes mC)
     (uint8->bytes mD)
     (uint8->bytes mE)
     (uint8->bytes mF))))

(define pkth-create-game-info
  (lambda (max-num-players raise-interval-mode raise-small-blind-interval raise-mode
           end-raise-mode proposed-gui-speed player-action-timeout
           first-small-blind end-raise-small-blind-value start-money manual-blind-slots)
    (append
     (uint16->bytes max-num-players)
     (uint16->bytes raise-interval-mode)
     (uint16->bytes raise-small-blind-interval)
     (uint16->bytes raise-mode)
     (uint16->bytes end-raise-mode)
     (uint16->bytes (length manual-blind-slots))
     (uint16->bytes proposed-gui-speed)
     (uint16->bytes player-action-timeout)
     (uint32->bytes first-small-blind)
     (uint32->bytes end-raise-small-blind-value)
     (uint32->bytes start-money)
     manual-blind-slots)))

(define pkth-create-player-info
  (lambda (player-id player-flags player-name avatar-md5)
    (append
     (uint32->bytes player-id)
     (uint16->bytes player-flags)
     (uint16->bytes (string-length player-name))
     (uint32->bytes 0)
     avatar-md5
     (append-padding (string->bytes player-name)))))

(define pkth-create-init-ex
  (lambda (version-major version-minor privacy-flags avatar-md5 password player-name)
    (pkth-create-packet
     pkth-type-init
     (list
      (uint16->bytes version-major)
      (uint16->bytes version-minor)
      (uint16->bytes (string-length password))
      (uint16->bytes (string-length player-name))
      (uint16->bytes privacy-flags)
      (uint16->bytes 0)
      avatar-md5
      (append-padding (string->bytes password))
      (append-padding (string->bytes player-name))))))

(define pkth-create-init
  (lambda (avatar-md5 password player-name)
    (pkth-create-init-ex
     pkth-version-major
     pkth-version-minor
     (if (null? avatar-md5) 0 pkth-privacy-flag-show-avatar)
     avatar-md5
     password
     player-name)))

#!
(pkth-create-init
 (pkth-create-md5 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
 ""
 "hallo")
(pkth-create-init
 '()
 ""
 "hallo")
!#

(define pkth-create-init-ack-ex
  (lambda (game-version beta-revision session-id player-id)
    (pkth-create-packet
     pkth-type-init-ack
     (list
      (uint16->bytes game-version)
      (uint16->bytes beta-revision)
      (uint32->bytes session-id)
      (uint32->bytes player-id)))))

(define pkth-create-init-ack
  (lambda (session-id player-id)
    (pkth-create-init-ack-ex
     pkth-game-version
     pkth-beta-revision
     session-id
     player-id)))

(define pkth-create-create-game
  (lambda (game-info game-name game-password)
    (pkth-create-packet
     pkth-type-create-game
     (list
      (uint16->bytes (string-length game-password))
      (uint16->bytes (string-length game-name))
      game-info
      (append-padding (string->bytes game-password))
      (append-padding (string->bytes game-name))))))

(define pkth-create-join-game
  (lambda (game-id game-password)
    (pkth-create-packet
     pkth-type-join-game
     (list
      (uint32->bytes game-id)
      (uint16->bytes (string-length game-password))
      (uint16->bytes 0)
      (append-padding (string->bytes game-password))))))

(define pkth-create-leave-current-game
  (lambda ()
    (pkth-create-packet
     pkth-type-leave-current-game
     (list
      (uint32->bytes 0)))))

(define pkth-create-start-event
  (lambda (start-flags)
    (pkth-create-packet
     pkth-type-start-event
     (list
      (uint16->bytes start-flags)
      (uint16->bytes 0)))))

(define pkth-create-start-event-ack
  (lambda ()
    (pkth-create-packet
     pkth-type-start-event-ack
     (list
      (uint32->bytes 0)))))

#!
(pkth-create-init-ack #x66666666 #x88888888)
!#

(define pkth-assert-minimal-length
  (lambda (message)
    (test-assert (>= (length message) pkth-minimum-message-length) "PKTH message is too small (no common header)!")))

#!
(pkth-assert-minimal-length '(1 2 3 4))
(pkth-assert-minimal-length '(1 2 3 4 5 6))
!#

(define pkth-assert-length
  (lambda (ls len)
    (test-assert (>= (length ls) len) "PKTH message is too small!")))

#!
(pkth-assert-length '(1 2 3 4 5 6 7 8) 16)
(pkth-assert-length '(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16) 16)
!#

(define pkth-is-valid-type?
  (lambda (message)
    (let ((type (pkth-get-type message)))
    (or
     (= type pkth-type-init)
     (= type pkth-type-init-ack)
     (= type pkth-type-retrieve-avatar)
     (= type pkth-type-avatar-header)
     (= type pkth-type-avatar-file)
     (= type pkth-type-avatar-end)
     (= type pkth-type-unknown-avatar)
     (= type pkth-type-game-list-new)
     (= type pkth-type-game-list-update)
     (= type pkth-type-game-list-player-joined)
     (= type pkth-type-game-list-player-left)
     (= type pkth-type-game-list-admin-changed)
     (= type pkth-type-retrieve-player-info)
     (= type pkth-type-player-info)
     (= type pkth-type-unknown-player-id)
     (= type pkth-type-create-game)
     (= type pkth-type-join-game)
     (= type pkth-type-join-game-ack)
     (= type pkth-type-join-game-failed)
     (= type pkth-type-player-joined)
     (= type pkth-type-player-left)
     (= type pkth-type-game-admin-changed)
     (= type pkth-type-kick-player)
     (= type pkth-type-leave-current-game)
     (= type pkth-type-start-event)
     (= type pkth-type-start-event-ack)
     (= type pkth-type-game-start)
     (= type pkth-type-hand-start)
     (= type pkth-type-players-turn)
     (= type pkth-type-players-action)
     (= type pkth-type-players-action-done)
     (= type pkth-type-players-action-rejected)
     (= type pkth-type-deal-flop-cards)
     (= type pkth-type-deal-turn-card)
     (= type pkth-type-deal-river-card)
     (= type pkth-type-all-in-show-cards)
     (= type pkth-type-end-of-hand-show-cards)
     (= type pkth-type-end-of-hand-hide-cards)
     (= type pkth-type-end-of-game)
     (= type pkth-type-statistics-changed)
     (= type pkth-type-removed-from-game)
     (= type pkth-type-send-chat-text)
     (= type pkth-type-chat-text)
     (= type pkth-type-error)))))

;;;
;;; Packet getter functions
;;;

(define pkth-get-type
  (lambda (message)
    (pkth-assert-minimal-length message)
    (let ((type (bytes->uint16 (list-head (list-tail message pkth-offset-type) pkth-length-type))))
      type)))

(define pkth-get-length
  (lambda (message)
    (pkth-assert-minimal-length message)
    (bytes->uint16 (list-head (list-tail message pkth-offset-msg-length) pkth-length-msg-length))))

(define pkth-get-data
  (lambda (message)
    (pkth-assert-minimal-length message)
    (list-tail message pkth-offset-data)))

;;; init-ack

(define pkth-get-init-ack-game-version
  (lambda (message)
    (pkth-assert-length message (+ pkth-header-length-common pkth-length-version))
    (bytes->uint16 (list-head (pkth-get-data message) pkth-length-version))))

(define pkth-get-init-ack-beta-revision
  (lambda (message)
    (pkth-assert-length message (+ pkth-header-length-common pkth-init-ack-offset-beta-revision pkth-length-revision))
    (bytes->uint16 (list-head (list-tail (pkth-get-data message) pkth-init-ack-offset-beta-revision) pkth-length-revision))))

(define pkth-get-init-ack-session-id
  (lambda (message)
    (pkth-assert-length message (+ pkth-header-length-common pkth-init-ack-offset-session-id pkth-length-session-id))
    (bytes->uint32 (list-head (list-tail (pkth-get-data message) pkth-init-ack-offset-session-id) pkth-length-session-id))))

(define pkth-get-init-ack-player-id
  (lambda (message)
    (pkth-assert-length message (+ pkth-header-length-common pkth-init-ack-offset-player-id pkth-length-player-id))
    (bytes->uint32 (list-head (list-tail (pkth-get-data message) pkth-init-ack-offset-player-id) pkth-length-player-id))))

;;;
;;; Type check functions
;;;

(define pkth-is-type-init?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-init)))

(define pkth-is-type-init-ack?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-init-ack)))

(define pkth-is-type-retrieve-avatar?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-retrieve-avatar)))

(define pkth-is-type-avatar-header?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-avatar-header)))

(define pkth-is-type-avatar-file?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-avatar-file)))

(define pkth-is-type-avatar-end?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-avatar-end)))

(define pkth-is-type-unknown-avatar?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-unknown-avatar)))

(define pkth-is-type-game-list-new?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-list-new)))

(define pkth-is-type-game-list-update?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-list-update)))

(define pkth-is-type-game-list-player-joined?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-list-player-joined)))

(define pkth-is-type-game-list-player-left?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-list-player-left)))

(define pkth-is-type-game-list-admin-changed?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-list-admin-changed)))

(define pkth-is-type-retrieve-player-info?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-retrieve-player-info)))

(define pkth-is-type-player-info?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-player-info)))

(define pkth-is-type-unknown-player-id?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-unknown-player-id)))

(define pkth-is-type-unknown-player-id?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-unknown-player-id)))

(define pkth-is-type-create-game?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-create-game)))

(define pkth-is-type-join-game?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-join-game)))

(define pkth-is-type-join-game-ack?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-join-game-ack)))

(define pkth-is-type-join-game-failed?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-join-game-failed)))

(define pkth-is-type-player-joined?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-player-joined)))

(define pkth-is-type-player-left?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-player-left)))

(define pkth-is-type-game-admin-changed?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-admin-changed)))

(define pkth-is-type-kick-player?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-kick-player)))

(define pkth-is-type-leave-current-game?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-leave-current-game)))

(define pkth-is-type-start-event?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-start-event)))

(define pkth-is-type-start-event-ack?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-start-event-ack)))

(define pkth-is-type-game-start?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-game-start)))

(define pkth-is-type-hand-start?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-hand-start)))

(define pkth-is-type-players-turn?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-players-turn)))

(define pkth-is-type-players-action?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-players-action)))

(define pkth-is-type-players-action-done?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-players-action-done)))

(define pkth-is-type-players-action-rejected?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-players-action-rejected)))

(define pkth-is-type-deal-flop-cards?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-deal-flop-cards)))

(define pkth-is-type-deal-turn-card?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-deal-turn-card)))

(define pkth-is-type-deal-river-card?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-deal-river-card)))

(define pkth-is-type-all-in-show-cards?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-all-in-show-cards)))

(define pkth-is-type-end-of-hand-show-cards?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-end-of-hand-show-cards)))

(define pkth-is-type-end-of-hand-hide-cards?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-end-of-hand-hide-cards)))

(define pkth-is-type-end-of-game?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-end-of-game)))

(define pkth-is-type-statistics-changed?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-statistics-changed)))

(define pkth-is-type-removed-from-game?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-removed-from-game)))

(define pkth-is-type-send-chat-text?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-send-chat-text)))

(define pkth-is-type-chat-text?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-chat-text)))

(define pkth-is-type-error?
  (lambda (message)
    (= (pkth-get-type message) pkth-type-error)))

;;; pkth-type-init

;;;
;;; I/O functions
;;;

;;; Connect to server according to config.
(define pkth-connect
  (lambda ()
    (set! helper-var-recv-buf "")
    (let ((sock (sock-create-tcp PKTH_CONF_CONNECT_ADDR_FAMILY)))
      (sock-bind sock PKTH_CONF_CONNECT_LOCAL_ADDR PKTH_CONF_CONNECT_LOCAL_PORT)
      (sock-connect sock PKTH_CONF_CONNECT_REMOTE_ADDR PKTH_CONF_CONNECT_REMOTE_PORT)
      sock)))

(define pkth-send-message-nolog
  (lambda (socket message)
    (sock-send socket (bytes->string message))))

(define pkth-send-message
  (lambda (socket message)
    (msg-display message helper-direction-send)
    (pkth-send-message-nolog socket message)))

; The recv function for PKTH is somewhat more complicated, because
; TCP has to be supported. We have to check for message boundaries.
(define pkth-recv-message
  (lambda (socket)
    (let ((ret 0))
      (do ((abort #f))
          (abort)
          (let ((buflen (string-length helper-var-recv-buf)))
            (if (>= buflen pkth-header-length-common)
                (begin
                  (let ((packetlen (pkth-get-length (string->bytes helper-var-recv-buf))))
                    (if (<= packetlen buflen)
                        (begin
                          (set! abort #t)
                          (let ((packet (string-copy (substring helper-var-recv-buf 0 packetlen))))
                            (set! helper-var-recv-buf (string-drop helper-var-recv-buf packetlen))
                            (set! ret (string->bytes packet))
                            (test-assert (pkth-is-valid-type? ret) "Invalid PKTH message type.")
                            (msg-display ret helper-direction-recv)
                            )))))))
          (if (not abort)
              (let ((buf (make-string pkth-buf-length)))
                (let ((recvret (sock-recv! socket buf)))
                  (let ((tmpbuf (substring buf 0 (car recvret))))
                    (if (string-null? tmpbuf) ; Abort if connection closed.
                        (begin
                          (set! helper-var-recv-buf "")
                          (set! abort #t)
                          (set! ret #f))
                        (begin
                          (set! helper-var-recv-buf (string-append helper-var-recv-buf tmpbuf))
                          )))))))
      ret)))

#!
(let ((sock (pkth-connect)))
  (pkth-send-message
   sock
   (pkth-create-init
    (pkth-create-md5 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
    ""
    "hallo"))
  (pkth-recv-message sock)
  (sleep 1)
  (pkth-close))
!#
