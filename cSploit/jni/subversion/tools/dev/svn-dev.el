;;;; Emacs Lisp help for writing Subversion code. ;;;;

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


;; Later on, there will be auto-detection of svn files, modeline
;; status, and a whole library of routines to interface with the
;; command-line client.  For now, there's this, at Ben's request.
;;
;; All this stuff should get folded into Emacs VC mode, really.

(defun svn-revert ()
  "Revert the current buffer and its file to its svn base revision."
  (interactive)
  (let ((obuf (current-buffer))
        (fname (buffer-file-name))
        (outbuf (get-buffer-create "*svn output*")))
    (set-buffer outbuf)
    (delete-region (point-min) (point-max))
    (call-process "svn" nil outbuf nil "status" fname)
    (goto-char (point-min))
    (search-forward fname)
    (beginning-of-line)
    (if (looking-at "^?")
        (error "\"%s\" is not a Subversion-controlled file" fname))
    (call-process "svn" nil outbuf nil "revert" fname)
    (set-buffer obuf)
    ;; todo: make a backup~ file?
    (save-excursion
      (revert-buffer nil t)
      (save-buffer))
    (message "Reverted \"%s\"." fname)))

(defun svn-resolved ()
  "Tell Subversion that conflicts in the current buffer and its file have
been resolved."
  (interactive)
  (let ((obuf (current-buffer))
        (fname (buffer-file-name))
        (outbuf (get-buffer-create "*svn output*")))
    (set-buffer outbuf)
    (delete-region (point-min) (point-max))
    (call-process "svn" nil outbuf nil "status" fname)
    (goto-char (point-min))
    (search-forward fname)
    (beginning-of-line)
    (if (looking-at "^?")
        (error "\"%s\" is not a Subversion-controlled file" fname))
    (call-process "svn" nil outbuf nil "resolved" fname)
    (set-buffer obuf)
    ;; todo: make a backup~ file?
    (save-excursion
      (revert-buffer nil t)
      (save-buffer))
    (message "Marked \"%s\" as conflict-free." fname)))

(defconst svn-adm-area ".svn"
  "The name of the Subversion administrative subdirectory.")

(defconst svn-adm-entries ".svn/entries"
  "The path from cwd to the Subversion entries file.")

(defun svn-controlled-path-p (path)
  "Return non-nil if PATH is under Subversion version control, else
return nil.  If PATH does not exist, return nil.

In the future, this will return an Emacs Lisp reflection of PATH's
entry, either an explicit svn-entry-struct, or a list of the form
\(LAST-COMMIT-REV CURRENT-REV LAST-COMMITTER ...\), so we can display
svn information in the mode line.  But that requires truly parsing the
entries file, instead of just detecting PATH among the entries."
  (interactive "f")  ; any use for interactive, other than testing?
  (cond
   ((not (file-exists-p path))
    nil)
   ((file-directory-p path)
    (let ((adm-area (concat path "/" svn-adm-area)))
      (if (file-directory-p adm-area)
          t
        nil)))
   (t
    (let ((entries  (concat (file-name-directory path) svn-adm-entries))
          (basename (file-name-nondirectory path))
          (found    nil))
      (save-excursion
	(if (file-directory-p (concat (file-name-directory path) svn-adm-area))
	    (progn
	      (let ((find-file-hooks nil))
		(set-buffer (find-file-noselect entries t)))
	      (goto-char (point-min))
	      (if (search-forward (format "name=\"%s\"" basename) nil t)
		  (setq found t)
		(setq found nil))
	      (kill-buffer nil)))
	found)))))


(defun svn-text-base-path (file)
  "Return the path to the text base for FILE (a string).
If FILE is a directory or not under version control, return nil."
  (cond
   ((not (svn-controlled-path-p file)) nil)
   ((file-directory-p file)            nil)
   (t
    (let* ((pdir (file-name-directory file))
           (base (file-name-nondirectory file)))
      (format "%s%s/text-base/%s.svn-base" (or pdir "") svn-adm-area base)))))


(defun svn-ediff (file)
  "Ediff FILE against its text base."
  (interactive "fsvn ediff: ")
  (let ((tb (svn-text-base-path file)))
    (if (not tb)
        (error "No text base for %s" file)
      (ediff-files file tb))))


(defun svn-find-file-hook ()
  "Function for find-file-hooks.
Inhibit backup files unless `vc-make-backup-files' is non-nil."
  (if (svn-controlled-path-p (buffer-file-name))
      (progn
        (if (string-match "XEMACS\\|XEmacs\\|xemacs" emacs-version)
            (vc-load-vc-hooks)) ; for `vc-make-backup-files'
        (unless vc-make-backup-files
          (make-local-variable 'backup-inhibited)
          (setq backup-inhibited t)))))

(add-hook 'find-file-hooks 'svn-find-file-hook)



;;; Dynamic generation of common Subversion URLs.
;;;
;;; (I have a version of this that actually fetches the stuff from the
;;; Net if you don't have a local copy, but it requires a very recent
;;; version of Emacs, so I didn't bother with it here.  -kfogel)

(defvar svn-site-source-tree-top (expand-file-name "~/projects/svn/site/")
  "*Top directory of your Subversion site source tree of
repository \"http://svn.apache.org/repos/asf/subversion/site\".
You almost certainly want to set this in your .emacs, to override
the default; use `(setq svn-site-source-tree-top
\"/path/to/the/site/tree\")'.")

(defvar svn-faq-file (concat svn-site-source-tree-top "/publish/faq.html")
  "*A local copy of the Subversion FAQ.")

(defvar svn-hacking-file (concat svn-site-source-tree-top 
                                 "/docs/community-guide/community-guide.html")
  "*A local copy of the Subversion hacking.html file.")

;; Helper for referring to issue numbers in a user-friendly way.
(defun svn-bug-url (n)
  "Insert the url for Subversion issue number N.  Interactively, prompt for N."
  (interactive "nSubversion issue number: ")
  (insert (format "http://subversion.tigris.org/issues/show_bug.cgi?id=%d" n)))

;; Helper for referring to revisions in a browser-friendly way.
(defun svn-rev-url (rev &optional transform)
  "Insert the url for Subversion revision REV, or if TRANSFORM is not
nil, then transform the revision at or around point into an HTML link.

Interactively, if at or inside a revision number, transform it into
full HTML link; otherwise, prompt for revision number and insert just
the resulting URL."
  (interactive (let ((thing (thing-at-point 'word)))
                 (if (and thing (string-match "r[0-9]+" thing))
                     (list thing t)
                   (list (read-string "Subversion revision number: ") nil))))
  (if (string-match "^r[0-9]+" rev)
      (setq rev (substring rev 1)))
  (if transform
      (let* ((bounds (bounds-of-thing-at-point 'word))
             (start (car bounds))
             (end   (cdr bounds)))
        (delete-region start end)))
  (insert (format "http://svn.apache.org/viewcvs?view=revision&revision=%s" 
                  rev)))

(defconst svn-url-base "http://subversion.apache.org/")
(defconst svn-faq-url (concat svn-url-base "faq.html"))
(defconst svn-hacking-url (concat svn-url-base 
                                  "docs/community-guide/community-guide.html"))

(defun svn-html-get-targets (file)
  "Build a list of targets for the Subversion web file FILE."
  (let* ((lst nil)
         (already-buffer (find-buffer-visiting file))
         (faq-buffer (or already-buffer (find-file-noselect file))))
    (save-excursion
      (set-buffer faq-buffer)
      (goto-char (point-min))
      ;; TODO: Ideally, this wouldn't depend on the presence of a
      ;; table of contents with "#" URLs, it would read the divs and
      ;; anchors themselves.
      (while (search-forward "href=\"#" nil t)
        (let ((b (point))
              (e (progn (search-forward "\"") (forward-char -1) (point))))
          (setq lst (cons (buffer-substring b e) lst))))
      (if (not already-buffer)
          (kill-buffer (current-buffer)))
      lst)))

(defun svn-url-completing-read (file prompt &optional hist-list)
  "Completingly read an HTML target for FILE, prompting with PROMPT.
If HIST-LIST is non-nil, it is a symbol: the completion history list to use."
  (progn
    (let* ((targets (svn-html-get-targets file))
           (target-str (completing-read prompt targets nil t nil hist-list)))
      (list target-str))))

(defvar svn-faq-history-list nil
  "History list for the 'svn-faq-url' prompt.")

(defvar svn-hacking-history-list nil
  "History list for the 'svn-hacking-url' prompt.")

(defun svn-faq-url (target)
  "Prompt with completion for a targeted SVN FAQ item, then insert it.
If called non-interactively, TARGET is the target within the faq (an
HTML anchor name, that is, the part after the \"#\")."
  (interactive
   (svn-url-completing-read svn-faq-file "FAQ entry: "
                            'svn-faq-history-list))
  (insert svn-faq-url "#" target))

(defun svn-hacking-url (target)
  "Prompt with completion for a targeted hacking.html item, then insert it.
If called non-interactively, TARGET is the target within hacking.html
(an HTML anchor name, that is, the part after the \"#\")."
  (interactive
   (svn-url-completing-read svn-hacking-file "hacking.html entry: "
                            'svn-hacking-history-list))
  (insert svn-hacking-url "#" target))



;;; Subversion C conventions
(if (eq major-mode 'c-mode)
    (progn
      (c-add-style "svn" '("gnu" (c-offsets-alist . ((inextern-lang . 0)))))
      (c-set-style "svn")))
(setq indent-tabs-mode nil)
(setq angry-mob-with-torches-and-pitchforks t)



;; Subversion Python conventions, plus some harmless helpers for
;; people who don't have python mode set up by default.
(autoload 'python-mode "python-mode" nil t)
(or (assoc "\\.py$" auto-mode-alist)
    (setq auto-mode-alist
          (cons '("\\.py$" . python-mode) auto-mode-alist)))

(defun svn-python-mode-hook ()
  "Set up the Subversion python conventions.  The effect of this is
local to the current buffer, which is presumably visiting a file in
the Subversion project.  Python setup in other buffers will not be
affected."
  (when (string-match "/subversion/" (buffer-file-name))
    (make-local-variable 'py-indent-offset)
    (setq indent-tabs-mode nil)
    (setq py-indent-offset 2)
    (make-local-variable 'py-smart-indentation)
    (setq py-smart-indentation nil)))

(add-hook 'python-mode-hook 'svn-python-mode-hook)



;; Much of the APR documentation is embedded perldoc format.  The
;; perldoc program itself sucks, however.  If you're the author of
;; perldoc, I'm sorry, but what were you thinking?  Don't you know
;; that there are people in the world who don't work in vt100
;; terminals?  If I want to view a perldoc page in my Emacs shell
;; buffer, I have to run the ridiculous command
;;
;;   $ PAGER=cat perldoc -t target_file
;;
;; (Not that this was documented anywhere, I had to figure it out for
;; myself by reading /usr/bin/perldoc).
;;
;; Non-paging behavior should be a standard command-line option.  No
;; program that can output text should *ever* insist on invoking the
;; pager.
;;
;; Anyway, these Emacs commands will solve the problem for us.
;;
;; Acknowledgements:
;; Much of this code is copied from man.el in the FSF Emacs 21.x
;; sources.

(defcustom svn-perldoc-overstrike-face 'bold
  "*Face to use when fontifying overstrike."
  :type 'face
  :group 'svn-dev)

(defcustom svn-perldoc-underline-face 'underline
  "*Face to use when fontifying underlining."
  :type 'face
  :group 'svn-dev)


(defun svn-perldoc-softhyphen-to-minus ()
  ;; \255 is some kind of dash in Latin-N.  Versions of Debian man, at
  ;; least, emit it even when not in a Latin-N locale.
  (unless (eq t (compare-strings "latin-" 0 nil
				 current-language-environment 0 6 t))
    (goto-char (point-min))
    (let ((str "\255"))
      (if enable-multibyte-characters
	  (setq str (string-as-multibyte str)))
      (while (search-forward str nil t) (replace-match "-")))))


(defun svn-perldoc-fontify-buffer ()
  "Convert overstriking and underlining to the correct fonts.
Same for the ANSI bold and normal escape sequences."
  (interactive)
  (message "Please wait, making up the page...")
  (goto-char (point-min))
  (while (search-forward "\e[1m" nil t)
    (delete-backward-char 4)
    (put-text-property (point)
		       (progn (if (search-forward "\e[0m" nil 'move)
				  (delete-backward-char 4))
			      (point))
		       'face svn-perldoc-overstrike-face))
  (goto-char (point-min))
  (while (search-forward "_\b" nil t)
    (backward-delete-char 2)
    (put-text-property (point) (1+ (point)) 'face svn-perldoc-underline-face))
  (goto-char (point-min))
  (while (search-forward "\b_" nil t)
    (backward-delete-char 2)
    (put-text-property (1- (point)) (point) 'face svn-perldoc-underline-face))
  (goto-char (point-min))
  (while (re-search-forward "\\(.\\)\\(\b\\1\\)+" nil t)
    (replace-match "\\1")
    (put-text-property (1- (point)) (point) 'face svn-perldoc-overstrike-face))
  (goto-char (point-min))
  (while (re-search-forward "o\b\\+\\|\\+\bo" nil t)
    (replace-match "o")
    (put-text-property (1- (point)) (point) 'face 'bold))
  (goto-char (point-min))
  (while (re-search-forward "[-|]\\(\b[-|]\\)+" nil t)
    (replace-match "+")
    (put-text-property (1- (point)) (point) 'face 'bold))
  (svn-perldoc-softhyphen-to-minus)
  (message "Please wait, making up the page...done"))


(defun svn-perldoc-cleanup-buffer ()
  "Remove overstriking and underlining from the current buffer."
  (interactive)
  (message "Please wait, cleaning up the page...")
  (progn
    (goto-char (point-min))
    (while (search-forward "_\b" nil t) (backward-delete-char 2))
    (goto-char (point-min))
    (while (search-forward "\b_" nil t) (backward-delete-char 2))
    (goto-char (point-min))
    (while (re-search-forward "\\(.\\)\\(\b\\1\\)+" nil t) 
      (replace-match "\\1"))
    (goto-char (point-min))
    (while (re-search-forward "\e\\[[0-9]+m" nil t) (replace-match ""))
    (goto-char (point-min))
    (while (re-search-forward "o\b\\+\\|\\+\bo" nil t) (replace-match "o"))
    (goto-char (point-min))
    (while (re-search-forward "" nil t) (replace-match " ")))
  (goto-char (point-min))
  (while (re-search-forward "[-|]\\(\b[-|]\\)+" nil t) (replace-match "+"))
  (svn-perldoc-softhyphen-to-minus)
  (message "Please wait, cleaning up the page...done"))


;; Entry point to svn-perldoc functionality.
(defun svn-perldoc (file)
  "Run perldoc on FILE, display the output in a buffer."
  (interactive "fRun perldoc on file: ")
  (let ((outbuf (get-buffer-create 
                 (format "*%s PerlDoc*" (file-name-nondirectory file))))
        (savepg (getenv "PAGER")))
    (setenv "PAGER" "cat")  ;; for perldoc
    (save-excursion
      (set-buffer outbuf)
      (delete-region (point-min) (point-max))
      (call-process "perldoc" nil outbuf nil (expand-file-name file))
      (svn-perldoc-fontify-buffer)      
      (svn-perldoc-cleanup-buffer)
      ;; Clean out the inevitable leading dead space.
      (goto-char (point-min))
      (re-search-forward "[^ \i\n]")
      (beginning-of-line)
      (delete-region (point-min) (point)))
    (setenv "PAGER" savepg)
    (display-buffer outbuf)))



;;; Help developers write log messages.

;; How to use this: just run `svn-log-message'.  You might want to
;; bind it to a key, for example,
;;
;;   (define-key "\C-cl" 'svn-log-message)
;;
;; The log message will accumulate in a file.  Later, you can use
;; that file when you commit:
;;
;;   $ svn ci -F msg ...

(defun svn-log-path-derive (path)
  "Derive a relative directory path for absolute PATH, for a log entry."
  (save-match-data
    (let ((base (file-name-nondirectory path))
          (chop-spot (string-match
                      "\\(code/\\)\\|\\(src/\\)\\|\\(projects/\\)"
                      path)))
      (if chop-spot
          (progn
            (setq path (substring path (match-end 0)))
            ;; Kluge for Subversion developers.
            (if (string-match "subversion/" path)
                (substring path (+ (match-beginning 0) 11))
              path))
        (string-match (expand-file-name "~/") path)
        (substring path (match-end 0))))))


(defun svn-log-message-file ()
  "Return the name of the appropriate log message accumulation file.
Usually this is just the file `msg' in the current directory, but
certain areas are treated specially, for example, the Subversion
source tree."
  (save-match-data
    (if (string-match "subversion" default-directory)
        (concat (substring default-directory 0 (match-end 0)) "/msg")
      "msg")))


(defun svn-log-message (short-file-names)
  "Add to an in-progress log message, based on context around point.
If prefix arg SHORT-FILE-NAMES is non-nil, then use basenames only in
log messages, otherwise use full paths.  The current defun name is
always used.

If the log message already contains material about this defun, then put
point there, so adding to that material is easy.

Else if the log message already contains material about this file, put
point there, and push onto the kill ring the defun name with log
message dressing around it, plus the raw defun name, so yank and
yank-next are both useful.

Else if there is no material about this defun nor file anywhere in the
log message, then put point at the end of the message and insert a new
entry for file with defun.

See also the function `svn-log-message-file'."
  (interactive "P")
  (let ((this-file (if short-file-names
                       (file-name-nondirectory buffer-file-name)
                     (svn-log-path-derive buffer-file-name)))
        (this-defun (or (add-log-current-defun)
                        (save-excursion
                          (save-match-data
                            (if (eq major-mode 'c-mode)
                                (progn
                                  (if (fboundp 'c-beginning-of-statement-1)
                                      (c-beginning-of-statement-1)
                                    (c-beginning-of-statement))
                                  (search-forward "(" nil t)
                                  (forward-char -1)
                                  (forward-sexp -1)
                                  (buffer-substring
                                   (point)
                                   (progn (forward-sexp 1) (point)))))))))
        (log-file (svn-log-message-file)))
    (find-file log-file)
    (goto-char (point-min))
    ;; Strip text properties from strings
    (set-text-properties 0 (length this-file) nil this-file)
    (set-text-properties 0 (length this-defun) nil this-defun)
    ;; If log message for defun already in progress, add to it
    (if (and
         this-defun                        ;; we have a defun to work with
         (search-forward this-defun nil t) ;; it's in the log msg already
         (save-excursion                   ;; and it's about the same file
           (save-match-data
             (if (re-search-backward  ; Ick, I want a real filename regexp!
                  "^\\*\\s-+\\([a-zA-Z0-9-_.@=+^$/%!?(){}<>]+\\)" nil t)
                 (string-equal (match-string 1) this-file)
               t))))
        (if (re-search-forward ":" nil t)
            (if (looking-at " ") (forward-char 1)))
      ;; Else no log message for this defun in progress...
      (goto-char (point-min))
      ;; But if log message for file already in progress, add to it.
      (if (search-forward this-file nil t)
          (progn 
            (if this-defun (progn
                             (kill-new (format "(%s): " this-defun))
                             (kill-new this-defun)))
            (search-forward ")" nil t)
            (if (looking-at " ") (forward-char 1)))
        ;; Found neither defun nor its file, so create new entry.
        (goto-char (point-max))
        (if (not (bolp)) (insert "\n"))
        (insert (format "\n* %s (%s): " this-file (or this-defun "")))
        ;; Finally, if no derived defun, put point where the user can
        ;; type it themselves.
        (if (not this-defun) (forward-char -3))))))



;;; Log message helpers.

(defconst svn-log-msg-sep-line
  "------------------------------------------------------------------------"
  "The line of dashes that separates log messages in 'svn log' output.")

(defconst svn-log-msg-boundary-regexp
  (concat "^" svn-log-msg-sep-line "\n" "r[0-9]+ | ")
  "Regular expression matching the start of a log msg.  The start is
the beginning of the separator line, not the rev/author/date line that
follows the separator line.")

(defun svn-narrow-to-log-msg ()
  "Narrow to the current Subversion log message.
This meant to be used while browsing the output of 'svn log'.
If point is not in such output, error."
  (interactive)
  (let ((start nil) (end nil))
    (save-excursion
      (re-search-backward svn-log-msg-boundary-regexp)
      (forward-line 1)
      (setq start (point))
      (end-of-line)
      (re-search-backward "| \\([0-9]+\\) ")
      (let ((num (match-string 1)))
        (re-search-forward "^\n")
        (forward-line (string-to-number num)))
      (setq end (point)))
    (narrow-to-region start end)))



(message "loaded svn-dev.el")
