;;; 
;;; Copyright (C) 2004, 2005 M. Tuexen tuexen@fh-muenster.de
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

;;; $Id: common.scm,v 1.4 2006/01/18 01:00:30 tuexen Exp $

;;; Load the SCTP API needed.
(use-modules (net sctp))

;;; Just have a convenient way of simple looping.
(use-modules (ice-9 syncase))
(define-syntax dotimes 
   (syntax-rules () 
     ((_ (var n res) . body) 
      (do ((limit n) 
           (var 0 (+ var 1))) 
          ((>= var limit) res) 
        . body)) 
     ((_ (var n) . body) 
      (do ((limit n) 
           (var 0 (+ var 1))) 
          ((>= var limit)) 
        . body))))

;;; The following functions implement modulo arithmetic.
(define 2^8  (expt 2 8))
(define 2^16 (expt 2 16))
(define 2^24 (expt 2 24))
(define 2^32 (expt 2 32))

(define 2^8-1  (1- 2^8))
(define 2^16-1 (1- 2^16))
(define 2^24-1 (1- 2^24))
(define 2^32-1 (1- 2^32))

(define (+mod2^8 x y)
  (modulo (+ x y) 2^8))
(define (-mod2^8 x y)
  (modulo (- x y) 2^8))
(define (*mod2^8 x y)
  (modulo (* x y) 2^8))

(define (+mod2^16 x y)
  (modulo (+ x y) 2^16))
(define (-mod2^16 x y)
  (modulo (- x y) 2^16))
(define (*mod2^16 x y)
  (modulo (* x y) 2^16))

(define (+mod2^24 x y)
  (modulo (+ x y) 2^24))
(define (-mod2^24 x y)
  (modulo (- x y) 2^24))
(define (*mod2^24 x y)
  (modulo (* x y) 2^24))

(define (+mod2^32 x y)
  (modulo (+ x y) 2^32))
(define (-mod2^32 x y)
  (modulo (- x y) 2^32))
(define (*mod2^32 x y)
  (modulo (* x y) 2^32))

;;; The following functions convert unsigned integers into
;;; a list of bytes in network byte order.
 
(define (uint8->bytes n)
  (if (and (exact? n) (integer? n) (<= 0 n 2^8-1))
      (list n)
      (error "Argument not a uint8" n)))

;;;(uint8->bytes 1)
;;;(uint8->bytes -1)
;;;(uint8->bytes 2^8)
;;;(uint8->bytes 2.0)

(define (uint16->bytes n)
  (if (and (exact? n) (integer? n) (<= 0 n 2^16-1))
      (list (quotient  n 2^8)
	    (remainder n 2^8))
      (error "Argument not a uint16" n)))

;;;(uint16->bytes 1)
;;;(uint16->bytes 2^8)
;;;(uint16->bytes 2^16)
;;;(uint16->bytes 2^16-1)

(define (uint24->bytes n)
  (if (and (exact? n) (integer? n) (<= 0 n 2^24-1))
      (list (quotient  n 2^16)
	    (quotient (remainder n 2^16) 2^8)
	    (remainder n 2^8))
      (error "Argument not a uint24", n)))

;;;(uint24->bytes 1)
;;;(uint24->bytes 2^8)
;;;(uint24->bytes 2^16)
;;;(uint24->bytes 2^24-1)

(define (uint32->bytes n)
  (if (and (exact? n) (integer? n) (<= 0 n 2^32-1))
      (list (quotient  n 2^24)
	    (quotient (remainder n 2^24) 2^16)
	    (quotient (remainder n 2^16) 2^8)
	    (remainder n 2^8))
      (error "Argument not a uint32", n)))

;;;(uint32->bytes 1)
;;;(uint32->bytes 2^8)
;;;(uint32->bytes 2^16)
;;;(uint32->bytes 2^24)
;;;(uint32->bytes 2^32-1)

(define uint8->big-endian-bytes uint8->bytes)
(define uint16->big-endian-bytes uint16->bytes)
(define uint24->big-endian-bytes uint24->bytes)
(define uint32->big-endian-bytes uint32->bytes)

(define (uint8->little-endian-bytes n)
  (reverse (uint8->bytes n)))

(define (uint16->little-endian-bytes n)
  (reverse (uint16->bytes n)))

(define (uint24->little-endian-bytes n)
  (reverse (uint24->bytes n)))

(define (uint32->little-endian-bytes n)
  (reverse (uint32->bytes n)))

;;;(uint32->little-endian-bytes 1024)


;;; The following functions converts the first bytes of the argument
;;; to an unsigned integer in host byte order.  

(define (bytes->uint8 l)
  (car l))

;;;(bytes->uint8 (uint8->bytes 56))

(define (bytes->uint16 l)
  (+ (* 2^8 (car l))
     (cadr l)))

;;;(bytes->uint16 (uint16->bytes 12345))

(define (bytes->uint24 l)
  (+ (* 2^16 (car l))
     (* 2^8 (cadr l))
     (caddr l)))

;;;(bytes->uint24 (uint24->bytes 12345567))

(define (bytes->uint32 l)
  (+ (* 2^24 (car l))
     (* 2^16 (cadr l))
     (* 2^8  (caddr l))
     (cadddr l)))

;;;(bytes->uint32 (uint32->bytes 2^32-1))

(define (list-head l n)
  (list-head-1 l n (list)))

(define (list-head-1 l n r)
  (if (<= n 0)
      (reverse r)
      (list-head-1 (cdr l) (- n 1) (cons (car l) r)))) 
;;; (list-head (list 1 2 3) 4)

(define big-endian-bytes->uint8 bytes->uint8)
(define big-endian-bytes->uint16 bytes->uint16)
(define big-endian-bytes->uint24 bytes->uint24)
(define big-endian-bytes->uint32 bytes->uint32)

(define (little-endian-bytes->uint8 l)
  (bytes->uint8 (reverse (list-head l 1))))

(define (little-endian-bytes->uint16 l)
  (bytes->uint16 (reverse (list-head l 2))))

(define (little-endian-bytes->uint24 l)
  (bytes->uint24 (reverse (list-head l 3))))

(define (little-endian-bytes->uint32 l)
  (bytes->uint32 (reverse (list-head l 4))))
;;;(little-endian-bytes->uint32 (uint32->little-endian-bytes 123456))

;;; This function generates a list of bytes representing a string.

(define (string->bytes s)
  (map char->integer (string->list s)))

;;;(string->bytes "Hello")

;;; Convert a list of bytes to a string which can be used by the send call

(define (bytes->string l)
  (list->string (map integer->char l)))

;;; (bytes->string '(65 65 65 0 65))

;;; This function generates a list of random bytes of a given length

(define (random-bytes n)
  (random-bytes-1 n (list)))

;;; This is the tail-recursive version 

(define (random-bytes-1 n l)
  (if (<= n 0)
      l
      (random-bytes-1 (- n 1) (cons (random 2^8) l))))

;;; (random-bytes 10000)

(define (zero-bytes n)
  (zero-bytes-1 n (list)))

(define (zero-bytes-1 n l)
  (if (<= n 0)
      l
      (zero-bytes-1 (- n 1) (cons 0 l))))

;;;(length (zero-bytes 3400))
;;;(zero-bytes 0)

(define (remove pred lst)
  (if (null? lst)
      (list)
      (if (pred (car lst))
	  (remove pred (cdr lst))
	  (cons (car lst) (remove pred (cdr lst))))))
;;; (remove positive? (list 1 -32 3 -9))
;;; (remove positive? (list -9))
;;; (remove positive? (list 1 2 3))

(define (filter pred lst)
  (if (null? lst)
      (list)
      (if (pred (car lst))
	  (cons (car lst) (filter pred (cdr lst)))
	  (filter pred (cdr lst)))))
;;; (filter positive? (list 1 -32 3 -9))
;;; (filter positive? (list -9))
;;; (filter positive? (list 1 2 3))


