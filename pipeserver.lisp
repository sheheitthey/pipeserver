(defstruct pipeserver-event
  event
  conn
  data)

; Read an event from STDIN
(defun pipeserver-read-event()
  (pipeserver-input-event (read-line)))

; Write an event to STDOUT
(defun pipeserver-write-event(e)
  (let ((s (pipeserver-output-event e)))
    (if s (format t "~A~%" s))))

; Send a line to a client
(defun pipeserver-send(conn str)
  (let ((e (make-pipeserver-event :event "SEND"
                                  :conn conn
                                  :data str)))
    (pipeserver-write-event e)
    e))

; Shutdown a client
(defun pipeserver-shutdown(conn)
  (let ((e (make-pipeserver-event :event "SHUTDOWN"
                                  :conn conn)))
    (pipeserver-write-event e)
    e))

; Close a client
(defun pipeserver-close(conn)
  (let ((e (make-pipeserver-event :event "CLOSE"
                                  :conn conn)))
    (pipeserver-write-event e)
    e))

; Interpret a string representing an incoming event into a pipeserver-event
; object
(defun pipeserver-input-event(str)
  (let ((e (pipeserver-verify-incoming-event
            (pipeserver-input-event-str str))))
    (if e e nil)))

; Create the string representation of an outgoing event from a pipeserver-event
; object
(defun pipeserver-output-event(e)
  (if (pipeserver-verify-outgoing-event e)
      (let ((event (pipeserver-event-event e))
            (conn (pipeserver-event-conn e))
            (data (pipeserver-filter-data (pipeserver-event-data e))))
        (if (equal event "SEND")
            (format nil "~A ~A ~A" event conn data)
          (format nil "~A ~A" event conn)))))

; Stores unverified/unconverted strings into a pipeserver-event structure
(defun pipeserver-input-event-str(str)
  (let ((line-regexp "^\\([A-Z]\\+\\) \\([0-9]\\+\\) \\(.*\\)$")
        (nonline-regexp "^\\([A-Z]\\+\\) \\([0-9]\\+\\)$"))
    (flet
     ((test-regexp(regexp)
         (mapcar (lambda (match)
                   (regexp:match-string str match))
                 (multiple-value-list
                  (regexp:match regexp str)))))
     (let ((ret (test-regexp line-regexp)))
       (if ret (make-pipeserver-event :event (second ret)
                                      :conn (third ret)
                                      :data (fourth ret))
         (let ((ret (test-regexp nonline-regexp)))
           (if ret (make-pipeserver-event :event (second ret)
                                          :conn (third ret))
             nil)))))))

; Verifies and constructs an incoming event structure
(defun pipeserver-verify-incoming-event(e)
  (flet ((verify-event()
            (let ((event (pipeserver-event-event e))
                  (data (pipeserver-event-data e)))
              (flet ((test-event(str)
                        (equal event str)))
                    (if (or (and (null data)
                                 (or (test-event "ACCEPTED")
                                     (test-event "CLOSED")
                                     (test-event "CLOSING")
                                     (test-event "ERROR")))
                            (and data
                                 (test-event "RECEIVED")))
                        event
                      nil))))
         (verify-conn()
            (let ((conn (pipeserver-event-conn e)))
              (if (every #'digit-char-p conn)
                  (parse-integer conn)
                nil)))
         (verify-data()
            (let ((data (pipeserver-event-data e)))
              (pipeserver-filter-data data))))
        (if e
            (let ((event (verify-event))
                   (conn (verify-conn))
                   (data (verify-data)))
              (if (and event conn)
                  (make-pipeserver-event :event event
                                         :conn conn
                                         :data data)
                nil))
          nil)))

; Verifies and constructs an outgoing event structure
(defun pipeserver-verify-outgoing-event(e)
  (flet ((verify-event()
            (let ((event (pipeserver-event-event e))
                  (data (pipeserver-event-data e)))
              (flet ((test-event(str)
                        (equal event str)))
                    (if (and (stringp event)
                             (or (and (null data)
                                      (or (test-event "CLOSE")
                                          (test-event "SHUTDOWN")))
                                 (and data
                                      (test-event "SEND"))))
                        event
                      nil))))
         (verify-conn()
            (let ((conn (pipeserver-event-conn e)))
              (if (and (integerp conn)
                       (>= conn 0))
                  conn
                nil)))
         (verify-data()
            (let ((data (pipeserver-event-data e)))
              (if (stringp data)
                  data
                nil))))
        (if e
            (let ((event (verify-event))
                  (conn (verify-conn))
                  (data (verify-data)))
              (if (and event conn)
                  (make-pipeserver-event :event event
                                         :conn conn
                                         :data data)
                nil))
          nil)))

; Filter a string to be suitable to use in the pipeserver-data field
(defun pipeserver-filter-data(str)
  (remove-if (lambda (c)
               (or (equal c #\newline)
                   (equal c #\return)))
             str))
