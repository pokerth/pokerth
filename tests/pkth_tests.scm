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
    (dotimes (n 2048)
             (pkth-send-message
              sock
              (pkth-create-packet
               pkth-type-init
               (list
                (uint16->bytes pkth-version-major)
                (uint16->bytes pkth-version-minor)
                (uint16->bytes (string-length ""))
                (uint16->bytes (string-length "test"))
                (uint16->bytes 0)
                (uint16->bytes 0)
                (append-padding (string->bytes ""))
                (append-padding (string->bytes "test"))
                (make-list 1024 0)))))
    (display "Done.\n")
  ))
(set! pkth-test-list (test-register pkth-test-list "PKTH Test 1 Init with too large packets" pkth-test-init-packet-too-large))

(test-run-all pkth-test-list)
(pkth-close)
