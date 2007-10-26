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

(load "common.scm")

(define sock-create-tcp
  (lambda (af)
    (let ((s (socket af SOCK_STREAM 0)))
      (cons s af))))

(define sock-create-udp
  (lambda (af)
    (let ((s (socket af SOCK_DGRAM 0)))
      (cons s af))))

(define sock-create-sctp
  (lambda (af)
    (let ((s (socket af SOCK_STREAM IPPROTO_SCTP)))
      (cons s af))))

(define sock-create-sctp-1toM
  (lambda (af)
    (let ((s (socket af SOCK_DGRAM IPPROTO_SCTP)))
      (cons s af))))

(define sock-close
  (lambda (sock)
    (let ((s (car sock)))
      (close s))))

(define sock-select-read
  (lambda (sock timeout-msec)
    (let ((s (car sock)))
      (=
       (vector-length (car (select (vector s) (vector) (vector) (quotient timeout-msec 1000) (* 1000 (modulo timeout-msec 1000)))))
       1))))

#!
(sock-close (sock-create-sctp AF_INET))
(sock-close (sock-create-sctp AF_INET6))
!#

 (define ipv4-resolve
  (lambda (name)
    (inet-ntop AF_INET (car (vector-ref (gethostbyname name) 4)))))
 
(define sock-bind
  (lambda (sock local-addr local-port)
    (let ((af (cdr sock)))
      (setsockopt (car sock) SOL_SOCKET SO_REUSEADDR 1)
      (setsockopt (car sock) SOL_SOCKET SO_LINGER (cons 1 60))
      (bind (car sock) af (inet-pton af local-addr) local-port)
      sock)))

#!
(sock-close (sock-bind (sock-create-udp AF_INET) "127.0.0.1" 5555))
(sock-close (sock-bind (sock-create-udp AF_INET6) "::1" 5555))
(sock-close (sock-bind (sock-create-tcp AF_INET) "127.0.0.1" 5555))
(sock-close (sock-bind (sock-create-tcp AF_INET6) "::1" 5555))
(sock-close (sock-bind (sock-create-sctp AF_INET) "127.0.0.1" 5555))
(sock-close (sock-bind (sock-create-sctp AF_INET6) "::1" 5555))
!#

(define sock-bind-listen
  (lambda (sock local-addr local-port queuesize)
    (sock-bind sock local-addr local-port)
    (listen (car sock) queuesize)
    sock))

#!
(sock-close (sock-bind-listen (sock-create-sctp AF_INET) "127.0.0.1" 5555 5))
(sock-close (sock-bind-listen (sock-create-sctp AF_INET6) "::1" 5555 5))
!#

(define sock-accept
  (lambda (sock)
    (let ((clientinfo (accept (car sock))))
      (let ((sender (cdr clientinfo)))
        (cons (cons (car clientinfo) (cdr sock)) (cons (inet-ntop (cdr sock) (sockaddr:addr sender)) (sockaddr:port sender)))))))

 #!
(sock-accept (sock-bind-listen (sock-create-tcp AF_INET) "127.0.0.1" 5555 5))
(sock-accept (sock-bind-listen (sock-create-tcp AF_INET6) "::1" 5555 5))
!#

(define sock-connect
  (lambda (sock remote-addr remote-port)
    (let ((af (cdr sock)))
      (connect (car sock) af (inet-pton af remote-addr) remote-port)
      sock)))

 #!
(sock-close (sock-connect (sock-create-tcp AF_INET) (ipv4-resolve "www.google.de") 80))
!#

(define sock-send
  (lambda (sock buf)
    (send (car sock) buf)))

(define sock-send-sctp
  (lambda (sock stream ppid buf)
    (if (= (cdr sock) AF_INET6)
        (sctp-sendmsg (car sock) buf (htonl ppid) stream 0 0 AF_INET6 (inet-pton AF_INET6 "::0") 0)
        (sctp-sendmsg (car sock) buf (htonl ppid) stream 0 0 AF_INET INADDR_ANY 0)
    )))

(define sock-recv-sctp!
  (lambda (sock buf)
    (let ((ret (sctp-recvmsg! (car sock) buf)))
      (let ((info (list-ref ret 3))) ; Return number of bytes, stream # and PPID
        (list (car ret) (ntohs (car info)) (ntohl (list-ref info 2)))))))

 #!
(sock-send (car (sock-accept (sock-bind-listen (sock-create-tcp AF_INET) "127.0.0.1" 5555 5))) "Hallo")
!#

(define sock-sendto
  (lambda (sock buf remote-addr remote-port)
    (let ((af (cdr sock)))
      (sendto (car sock) buf af (inet-pton af remote-addr) remote-port))))

(define sock-recvfrom!
  (lambda (sock buf)
    (let ((ret (recvfrom! (car sock) buf)))
      (let ((sender (cdr ret)))
        (cons (car ret) (cons (inet-ntop (cdr sock) (sockaddr:addr sender)) (sockaddr:port sender)))))))

(define sock-recv!
  (lambda (sock buf)
    (let ((ret (recv! (car sock) buf)))
      (cons ret (cons "" 0)))))

#!
(let ((server (sock-bind (sock-create-udp AF_INET) "0.0.0.0" 4440)))
  (let ((client (sock-connect (sock-create-udp AF_INET) (ipv4-resolve "localhost") 4440)))
    (sock-send client "Test\0")
    (let ((recv-buf (make-string 10)))
      (sock-recv! server recv-buf)
      (display recv-buf)
      (sock-close server)
      (sock-close client))))
!#
