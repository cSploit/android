;;; svn-entries.el --- Display .svn/entries field names to the left

;; Copyright (C) 2007  David Glasser

;; Licensed under the same license as Subversion.

;; Licensed to the Apache Software Foundation (ASF) under one
;; or more contributor license agreements.  See the NOTICE file
;; distributed with this work for additional information
;; regarding copyright ownership.  The ASF licenses this file
;; to you under the Apache License, Version 2.0 (the
;; "License"); you may not use this file except in compliance
;; with the License.  You may obtain a copy of the License at
;;
;;   http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing,
;; software distributed under the License is distributed on an
;; "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
;; KIND, either express or implied.  See the License for the
;; specific language governing permissions and limitations
;; under the License.

;;; Commentary:

;; Display field names to the left of the lines in a .svn/entries
;; buffer. Copy svn-entries.el to your load-path and add to your
;; .emacs:

;;    (require 'svn-entries)

;; After opening or editing an entries file, run

;;   M-x svn-entries-show

;; To hide:

;;   M-x svn-entries-hide

;; (I tried doing this as a minor mode but setting margins during
;; alist initialization didn't work...)

;; Tested on FSF Emacs 22.


(defvar svn-entries-overlays nil "Overlays used in this buffer.")
(make-variable-buffer-local 'svn-entries-overlays)

(defgroup svn-entries nil
  "Show labels to the left of .svn/entries buffers"
  :group 'convenience)

(defface svn-entries
  '((t :inherit shadow))
  "Face for displaying line numbers in the display margin."
  :group 'svn-entries)

(defun svn-entries-set-margins (buf margin)
  (dolist (w (get-buffer-window-list buf nil t))
    (set-window-margins w margin)))

(defun svn-entries-hide ()
  "Delete all overlays displaying labels for this buffer."
  (interactive)
  (mapc #'delete-overlay svn-entries-overlays)
  (setq svn-entries-overlays nil)
  (svn-entries-set-margins (current-buffer) 0)
  (remove-hook 'window-configuration-change-hook
               'svn-entries-after-config t))

(defun svn-entries-show ()
  "Update labels for the current buffer."
  (interactive)
  (svn-entries-update (current-buffer))
  (add-hook 'window-configuration-change-hook
            'svn-entries-after-config nil t))

(defconst svn-entries-labels 
  ["name"
   "kind"
   "revision"
   "url"
   "repos"
   "schedule"
   "text-time"
   "checksum"
   "committed-date"
   "committed-rev"
   "last-author"
   "has-props"
   "has-prop-mods"
   "cachable-props"
   "present-props"
   "conflict-old"
   "conflict-new"
   "conflict-wrk"
   "prop-reject-file"
   "copied"
   "copyfrom-url"
   "copyfrom-rev"
   "deleted"
   "absent"
   "incomplete"
   "uuid"
   "lock-token"
   "lock-owner"
   "lock-comment"
   "lock-creation-date"
   "changelist"
   "keep-local"
   "working-size"
   "depth"])

(defconst svn-entries-margin-width (length "lock-creation-date"))

(defun svn-entries-update (buffer)
  "Update labels for all windows displaying BUFFER."
  (with-current-buffer buffer
    (svn-entries-hide)
    (save-excursion
      (save-restriction
        (widen)
        (let ((last-line (line-number-at-pos (point-max)))
              (field 0)
              (done nil))
          (goto-char (point-min))
          (while (not done)
            (cond ((= (point) 1)
                   (svn-entries-overlay-here "format"))
                  ((= (following-char) 12) ; ^L
                   (setq field 0))
                  ((not (eobp))
                   (svn-entries-overlay-here (elt svn-entries-labels field))
                   (setq field (1+ field))))
            (setq done (> (forward-line) 0))))))
    (svn-entries-set-margins buffer svn-entries-margin-width)))

(defun svn-entries-overlay-here (label)
  (let* ((fmt-label (propertize label 'face 'svn-entries))
         (left-label (propertize " " 'display `((margin left-margin)
                                                ,fmt-label)))
         (ov (make-overlay (point) (point))))
    (push ov svn-entries-overlays)
    (overlay-put ov 'before-string left-label)))

(defun svn-entries-after-config ()
  (walk-windows (lambda (w) (svn-entries-set-margins-if-overlaid (window-buffer))) 
                nil 'visible))

(defun svn-entries-set-margins-if-overlaid (b)
  (with-current-buffer b
    (when svn-entries-overlays
      (svn-entries-set-margins b svn-entries-margin-width))))

(provide 'svn-entries)
;;; svn-entries.el ends here
