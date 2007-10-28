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

;;; Test functions
(load "test.scm")
;;; Software timer
(load "timer.scm")

;;; Helper constants

(define helper-direction-recv               1)
(define helper-direction-send               2)

;;; Helper variables

(define helper-var-last-display-msg         '())
(define helper-var-last-display-direction   0)
(define helper-var-display-newline          #f)
(define helper-var-recv-buf                 "")

(define helper-init-vars
  (lambda ()
    (set! helper-var-last-display-msg '())
    (set! helper-var-last-display-direction 0)
    (set! helper-var-display-newline #f)
    ))

(define predicate?
  (lambda (pred data)
    (apply pred (list data))))

(define predicate-one-true?
  (lambda (predicate-list data)
    (let ((data-list (make-list (length predicate-list) data)))
      (primitive-eval (append '(or) (map predicate? predicate-list data-list))))))

(define wait-for-message
  (lambda (sock recv-function predicate-list ignore-predicate-list timeout-msec)
    (let ((t (timer-create timeout-msec)))
      (set! t (timer-start t))
      (do ((abort #f))
          (abort)
          (if (or (not (string-null? helper-var-recv-buf)) (sock-select-read sock 10))
              (begin
                (let ((msg (recv-function sock)))
                  (if (predicate-one-true? predicate-list msg)
                      (begin
                        (set! abort #t))
                      (begin
                        (if (not (predicate-one-true? ignore-predicate-list msg))
                            (begin
                              (test-assert #f "The upper message was received but not expected."))))
                      ))))
          (if (timer-expired? t)
              (begin
                (set! abort #t)
                (test-assert (null? predicate-list) "Expected message not received within time interval!"))))
      )))

(define wait-for-message-in-interval
  (lambda (sock recv-function predicate-list ignore-predicate-list delay-before-msec until-msec)
    (wait-for-message sock recv-function '() ignore-predicate-list delay-before-msec)
    (wait-for-message sock recv-function predicate-list ignore-predicate-list (- until-msec delay-before-msec))))

#!
(define recv-message
  (lambda (socket)
    (let ((buffer (make-string 256)))
      (let ((ret (sock-recv! socket buffer)))
        (let ((n (car ret)))
          (string->bytes (substring buffer 0 n)))))))

(define msg-test?
  (lambda (msg)
    #t))

(let ((sock (car (sock-accept (sock-bind-listen  (sock-create-tcp AF_INET) "127.0.0.1" 6002 5)))))
  (let ((ret (wait-for-message sock recv-message (list msg-test?) '() 10000)))
    (sleep 1)
    (sock-close sock)
    ret))
!#

(define msg-display
 (lambda (message direction)
   (if (and (equal? message helper-var-last-display-msg) (= direction helper-var-last-display-direction))
       (begin
         (display ".")
         (set! helper-var-display-newline #t))
       (begin
         (if helper-var-display-newline
             (begin
               (display "\n")
               (set! helper-var-display-newline #f)))
         (if (= direction helper-direction-recv)
             (display "<-")
             (display "->"))
         (set! helper-var-last-display-msg (list-copy message))
         (set! helper-var-last-display-direction direction)
         (display message)
         (display "\n")))))
 
;;;
;;; Padding helper functions
;;;

(define calc-num-padding-bytes
  (lambda (size)
    (remainder (- 4 (remainder size 4)) 4)))

(define append-padding
  (lambda (data)
    (append data (zero-bytes (calc-num-padding-bytes (length data))))))

#!
(append-padding (list 1 2 3 4 5))
!#
