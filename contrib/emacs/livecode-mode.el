;; Copyright (C) 2015 LiveCode Ltd.

;; This file is part of LiveCode.

;; LiveCode is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License v3 as published
;; by the Free Software Foundation.

;; LiveCode is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.mlc\\'" . livecode-mode))
(add-to-list 'auto-mode-alist '("\\.lcb\\'" . livecode-mode))

(defvar livecode-mode-syntax-table
  (let ((st (make-syntax-table)))
    ;; Comments
    (modify-syntax-entry ?/ ". 124" st)
    (modify-syntax-entry ?* ". 23b" st)
    (modify-syntax-entry ?\- ". 12" st)
    (modify-syntax-entry ?\n ">" st)
    ;; Angle brackets
    (modify-syntax-entry ?< "(>" st)
    (modify-syntax-entry ?> ")<" st)
    st)
  "Syntax table for LiveCode mode")

(defvar livecode-keywords
  '("variable" "syntax" "begin" "end" "phrase" "operator"
    "foreign" "handler" "prefix" "postfix" "precedence" "statement"
    "undefined" "public" "neutral" "binds to" "optional"
    "any" "private" "if" "else" "then" "repeat" "metadata" "widget" "module"
    "version" "author" "title" "true" "false" "return" "use" "type"
    "exit repeat" "next repeat"))

(defvar livecode-builtins
  '("output" "input"
    "bool" "boolean"
    "int" "number" "real" "double" "float"
    "string" "data" "list" "map" "pointer" "is" "empty" "as"))

(defvar livecode-font-lock-defaults
  (let ((symbol-regexp "\\_<\\(\\(?:\\s_\\|\\w\\)*\\)\\_>"))
    `((
       ;; stuff between "
       ("\"\\.\\*\\?" . font-lock-string-face)

       ;; Keywords and builtins
       ( ,(regexp-opt livecode-keywords 'symbols) . font-lock-keyword-face)
       ( ,(regexp-opt livecode-builtins 'symbols) . font-lock-builtin-face)

       ;; handler definitions including parameter names
       ( ,(concat "^\\s-*\\(\\_<\\(public\\|foreign\\)\\_>\\s-+\\)?\\_<handler\\_>\\s-+" symbol-regexp)
	 (3 font-lock-function-name-face nil)
	 ( ,(concat "\\(" (regexp-opt '("in" "out" "inout") 'symbols) "\\)"
		    "\\s-+" symbol-regexp)
	   nil nil
	   (3 font-lock-variable-name-face)
	   (2 font-lock-keyword-face))
	 ("\\_<as\\s-+\\(\\(?:\\s_\\|\\w\\)*\\)\\_>"
	  nil nil
	  (1 font-lock-type-face)))

       ;; variable names
       ( ,(concat "^\\s-*\\_<variable\\_>\\s-*" symbol-regexp)
	(1 font-lock-variable-name-face)
	("\\_<as\\s-+\\(\\(?:\\s_\\|\\w\\)*\\)\\_>"
	 nil nil
	 (1 font-lock-type-face)))

       ;; syntax definitions
       ( ,(concat "^\\s-*\\_<syntax\\_>\\s-+" symbol-regexp)
	 (1 font-lock-function-name-face)
	 ("\\_<is\\_>" nil nil (0 font-lock-keyword-face t)))
       ))))

(define-derived-mode livecode-mode prog-mode "LiveCode"
  "Major mode for editing LiveCode builder source files"
  :syntax-table livecode-mode-syntax-table

  (setq font-lock-defaults livecode-font-lock-defaults)

  (set (make-local-variable 'comment-start) "--")
  (set (make-local-variable 'comment-end) "")
  )

(provide 'livecode-mode)
