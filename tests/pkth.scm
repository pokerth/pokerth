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

;;; SCTP Payload Protocol Identifier for PKTH
(define pkth-ppid                               0)

;;; SCTP Port for PKTH
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
(define pkth-init-ack-offset-session-id         (+ pkth-init-ack-offset-beta-revision pkth-length-version))
(define pkth-init-ack-offset-player-id          (+ pkth-init-ack-offset-session-id pkth-length-session-id))

;;; Minimum/maximum packet length
(define pkth-minimum-message-length             8)
(define pkth-maximum-message-length             268)

;;;
;;; Header constructors
;;;

(define (pkth-create-packet type data)
  (append
   (uint16->bytes type)
   (uint16->bytes (+ pkth-header-length-common (apply + (map length data))))
   (apply append data)))

(define (pkth-create-md5 m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 mA mB mC mD mE mF)
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
   (uint8->bytes mF)))

(define (pkth-create-init-ex version-major version-minor privacy-flags avatar-md5 password player-name)
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
    (append-padding (string->bytes player-name)))))

(define (pkth-create-init avatar-md5 password player-name)
  (pkth-create-init-ex
   pkth-version-major
   pkth-version-minor
   (if (null? avatar-md5) 0 pkth-privacy-flag-show-avatar)
   avatar-md5
   password
   player-name))

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

(define (pkth-create-init-ack-ex game-version beta-revision session-id player-id)
  (pkth-create-packet
   pkth-type-init-ack
   (list
    (uint16->bytes game-version)
    (uint16->bytes beta-revision)
    (uint32->bytes session-id)
    (uint32->bytes player-id))))

(define (pkth-create-init-ack session-id player-id)
  (pkth-create-init-ack-ex
   pkth-game-version
   pkth-beta-revision
   session-id
   player-id))

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

;;;
;;; Packet getter functions
;;;

(define pkth-get-type
  (lambda (message)
    (pkth-assert-minimal-length message)
    (bytes->uint16 (list-head (list-tail message pkth-offset-type) pkth-length-type))))

(define pkth-get-length
  (lambda (message)
    (pkth-assert-minimal-length message)
    (bytes->uint16 (list-head (list-tail message pkth-offset-msg-length) pkth-length-msg-length))))

(define pkth-get-data
  (lambda (message)
    (pkth-assert-minimal-length message)
    (list-tail message pkth-offset-data)))

;;; pkth-type-init

#!
(let ((sock (sock-connect (sock-create-tcp AF_INET) "127.0.0.1" pkth-port)))
  (sock-send
   sock
   (bytes->string
    (pkth-create-init
     (pkth-create-md5 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
     ""
     "hallo")))
  (sleep 1)
  (sock-close sock))
!#
