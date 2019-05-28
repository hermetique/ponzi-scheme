(define (not b)
  (if b #f #t))

(define = eq?)
(define char=? =)

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (box x) (cons 'box x))
(define (unbox x) (cdr x))
(define (set-box! box val) (set-cdr! box val))

(define macros (box '()))

(define (push-macro! name expander)
  (set-box! macros (cons (cons name expander) (unbox macros))))

(define (lookup-macro-in symbol table)
  (if (null? table)
      #f
      (if (= (caar table) symbol)
        (cdar table)
        (lookup-macro-in symbol (cdr table)))))

(define (make-lambda args body)
  (cons 'lambda (cons args body)))

(define (map f x)
  (if (null? x)
      '()
      (cons (f (car x))
            (map f (cdr x)))))

(push-macro! 'let
  (lambda (macro-arguments)
    ((lambda (args vals body)
       (cons (make-lambda args body) vals))
     (map car (car macro-arguments))
     (map cadr (car macro-arguments))
     (cdr macro-arguments))))

(define (member x xs)
  (if (null? xs)
      #f
      (if (= (car xs) x)
          #t
          (member x (cdr xs)))))

(define (append xs ys)
  (if (null? xs)
      ys
      (cons (car xs) (append (cdr xs) ys))))

(define (expand/helper shadow s)
  (if (if (pair? s)
        (if (symbol? (car s))
          (not (member (car s) shadow))
          #f)
        #f)
   (if (eq? (car s) 'quote)
     s
     (if (eq? (car s) 'lambda)
       (make-lambda (cadr s)
                    (expand/helper (append (cadr s) shadow)
                                   (cddr s)))
       ((lambda (m-entry)
          (if m-entry
            (expand/helper shadow (m-entry (cdr s)))
            (cons (expand/helper shadow (car s))
                  (expand/helper shadow (cdr s)))))
        (lookup-macro-in (car s) (unbox macros)))))
   (if (pair? s)
     (cons (expand/helper shadow (car s))
           (expand/helper shadow (cdr s)))
     s)))

(define (expand s)
  (expand/helper '() s))

(define (apply-macro-expander f xs)
  (eval (cons f (map (lambda (x) (list 'quote x)) xs)) (environment)))

(push-macro! 'define-syntax
  (lambda (macro-arguments)
    (let ((name (car macro-arguments))
          (body (cdr macro-arguments)))
      (if (pair? name)
        (let ((name (car name))
              (args (cdr name)))
          (list
            'push-macro!
            (list 'quote name)
            (make-lambda (list 'macro-arguments)
                         (list (list 'apply-macro-expander (make-lambda args body)
                                     'macro-arguments)))))
        (list 'push-macro! (list 'quote name) (car body))))))

(define (and-expander macro-arguments)
  (if (null? macro-arguments)
    #t
    (list
      'if
      (car macro-arguments)
      (and-expander (cdr macro-arguments))
      #f)))

(define (or-expander macro-arguments)
  (if (null? macro-arguments)
    #t
    (list
      'if
      (car macro-arguments)
      #t
      (or-expander (cdr macro-arguments)))))

(push-macro! 'and and-expander)
(push-macro! 'or or-expander)

(define (quasiquote/helper expr)
  (if (and (pair? expr)
           (= 'unquote (car expr)))
    (cadr expr)
    (if (pair? expr)
      (list 'cons
        (quasiquote/helper (car expr))
        (quasiquote/helper (cdr expr)))
      (list 'quote expr))))

(push-macro! 'quasiquote
  (lambda (args)
    (if (and (pair? args) (null? (cdr args)))
      (quasiquote/helper (car args))
      (error "bad quasiquote"))))

(define-syntax begin
  (lambda (body)
    `(,(make-lambda '() body))))

(define-syntax unless
  (lambda (macro-arguments)
    (let ((condition (car macro-arguments))
          (body (cdr macro-arguments)))
      `(if ,condition (begin . ,body) #f))))

(define-syntax unless
  (lambda (macro-arguments)
    (let ((condition (car macro-arguments))
          (body (cdr macro-arguments)))
      `(if ,condition #f (begin . ,body)))))

(define (cond-expand exprs)
  (if (and (pair? exprs) (not (null? exprs))
           (pair? (car exprs)))
      `(if ,(caar exprs)
           (,(make-lambda '() (cdar exprs)))
           ,(cond-expand (cdr exprs)))
      (if (null? exprs) '#f)))

(define else #t)

(push-macro! 'cond cond-expand)

(define (set!-helper env symbol value)
  (cond
    ((and (not (null? env))
          (eq? (caar env) symbol))
     (set-cdr! (car env) value))
    ((not (null? env))
     (set!-helper (cdr env) symbol value))
    (else
      (write "no existing binding for " symbol))))

(define-syntax (set! symbol value)
  `(set!-helper (environment) ',symbol ,value))

(define-syntax letrec
  (lambda (macro-arguments)
    (define (make-set entry)
      `(set! ,(car entry) (lambda ,(cadr entry) . ,(cddr entry))))
    (let ((definitions (car macro-arguments))
          (body (cdr macro-arguments)))
      (cons (make-lambda (map car definitions)
                         (append (map make-set definitions)
                                 body))
        (map (lambda () 'letrec-unbound-definition) definitions)))))

(define (push! list value)
  (set-cdr! list (cons value (cdr list))))
