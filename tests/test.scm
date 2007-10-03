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

;;;
;;; Basic check functions
;;;

(define test-get-next-test-num
  (lambda (test-list)
    (+ (length test-list) 1)))

;;; Simple test registration
(define test-register
  (lambda (test-list name function)
    (append test-list (list (cons (cons (test-get-next-test-num test-list) name) function)))))

(define test-assert
  (lambda (predicate errortext)
    (if (not predicate)
        (begin
          (display (string-append "Assertion failed: " errortext "\n"))
          (throw 'badex)))))

#!
(test-assert #f "Test")
(test-assert #t "Test")
!#

(define test-get-string
  (lambda (data)
    (string-append "Test " (number->string (car (car data))) ": \"" (cdr (car data)) "\"")))

(define test-run-single
  (lambda (data)
    (display (string-append "Running " (test-get-string data) "...\n"))
    (catch 'badex
        (lambda ()
          ((cdr data)) ; call test
          (display "PASSED\n"))
      (lambda (key)
        (display "FAILED\n")))))

(define test-run-all
  (lambda (test-list)
    (display "\nTest Run: All Tests\n")
    (map test-run-single test-list)
    (display "\nDone.\n")))

#!
(define test-func
  (lambda ()
    (test-assert #f "False should be true!")))
(test-register "Test" test-func)
(test-run-all)
!#
