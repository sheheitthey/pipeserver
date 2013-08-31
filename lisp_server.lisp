(require "pipeserver")

;(defun conditional-eval(expr)
;  (handler-case (eval expr)
;                (error (condition) condition)))

(defun str-conditional-eval-str(str)
  (handler-case (format nil "~S" (eval (read-from-string str)))
                (error (condition) (format nil "~A" condition))))

(defun handle-received(conn data)
  (if (equal data "(exit)")
      (pipeserver-shutdown conn)
    (pipeserver-send conn (str-conditional-eval-str data))))
    ;(pipeserver-send conn data)))

(defun lisp-server()
  (let ((e (pipeserver-read-event)))
    (if e
        (let ((event (pipeserver-event-event e))
              (conn (pipeserver-event-conn e))
              (data (pipeserver-event-data e)))
          (cond ((equal event "RECEIVED")
                 (handle-received conn data))
                ((equal event "CLOSING")
                 (pipeserver-shutdown conn)))))
    (lisp-server)))

(defun broadcast-str(max fn-conn)
  (do ((conn 0 (+ conn 1))) ((>= conn max))
      (pipeserver-send conn (funcall fn-conn conn))))

(defun broadcast-connection-str(conn)
  (format nil "You are connection number: ~A." conn))

(defun broadcast-connections(max)
  (broadcast-str max #'broadcast-connection-str))

;(defun broadcast-connections(max)
;  (do ((conn 0 (+ conn 1))) ((> conn max))
;      (pipeserver-send conn
;                       (format nil "You are connection number: ~A." conn))))

(lisp-server)
