#|
LambdaNative - a cross-platform Scheme framework
Copyright (c) 2025, John Agoe
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the following
disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials
provided with the distribution.

* Neither the name of the University of British Columbia nor
the names of its contributors may be used to endorse or
promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|#


;;(c-declare "#include \"main.c\"")
(define gui #f)
(define text #f)
(define img-box #f)
(define width 402)
(define height 874)
(define padding 20)


(define vh(lambda(h)
    (set! h (/ h 100))
    (set! h (* h height))
    (+ h 0)
))

(define vw(lambda(w)
    (set! w (/ w 100))
    (set! w (* w width))
    (+ w 0)
))

(define center(lambda(w)
    (set! w (- width w))
    (set! w (/ w 2))
    (+ w 0)
))

(define result (lambda(t p)
    (glgui-box gui (center (- width padding)) (vh p) (- width padding) (vh 7) Blue)
    (glgui-box gui (+ (center (- width padding)) 1) (+ (vh p) 1) (- (- width padding) 2) (- (vh 7) 2) DarkGrey)
    (set! text (glgui-label gui (+ (center (- width padding)) (vh 10)) (vh p) (- width padding) (vh 7) t dejavu_16.fnt Blue))
))

(main
    (lambda (w h)
        (make-window width height)
        (glgui-orientation-set! GUI_PORTRAIT)

        (set! gui (make-glgui))
        (glgui-box gui 0 0 width height Grey)
        (glgui-box gui (center (vh 35)) (vh 5) (vh 35) (vh 10) Blue)
        (glgui-label gui (center 250) (vh 5) (vh 35) (vh 10) "Start Logging" dejavubold_32.fnt White)
        (result "Wireless Mouse" 33)
        (result "PS4 Controller" 25) 
        (result "HID Keyboard" 17)
        (glgui-label gui (center (- width 20)) (vh 38) width (vh 10) "(Click on a device to view its data)" dejavu_16.fnt Blue)
        (glgui-box gui (center (vw 90)) (- (- height (vh 50)) padding) (vw 90) (vh 50) White)
        (glgui-label gui (center (vw 90)) (- (- height (vh 50)) padding) (vw 90) (vh 50) "Live log data will appear here..." dejavu_16.fnt Black)
    )

    (lambda (t x y)
        (if (= t EVENT_KEYPRESS) (begin
        (if (= x EVENT_KEYESCAPE) (terminate))))
        (glgui-event gui t x y)
    )

    (lambda () #t)

    (lambda () (glgui-suspend))

    (lambda () (glgui-resume))
)