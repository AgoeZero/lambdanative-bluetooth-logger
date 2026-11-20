(define gui #f)
(define width 720)
(define height 1280)
(define padding 20)
(define theme-bg DarkBlue)
(define theme-fg White)
(define device-selector #f)
(set! cursor 0)

(define vh(lambda(h)
    (set! h (/ h 100))
    (set! h (* h height))
    (+ h 0)
))

(define center(lambda(w)
    (set! w (- width w))
    (set! w (/ w 2))
    (+ w 0)
))

(define (make-device-entry name)
    (lambda( g wgt x y w h selected)
        (if selected (glgui:draw-box x y w h theme-fg))
        (glgui:draw-text-left (+x 5) y (- w 10) h name dejavu_16.fnt theme-bg)
    )
)

(main
    (lambda (w h)
        (make-window width height)
        (glgui-orientation-set! GUI_PORTRAIT)
        (set! gui (make-glgui))
        (glgui-box gui 0 0 width height theme-bg)
              
        ;;draw the title bar
        (glgui-box gui 0 (vh 90) width (vh 10) theme-fg)
        (glgui-label gui 0 (vh 90) width (vh 10) " Bluetooth Logger" dejavubold_32.fnt theme-bg)
        
        (glgui-box gui padding (vh 55) (- width (* 2 padding)) (vh 30) theme-fg)
        (glgui-label gui padding (vh 40) width (vh 10) " Paired Devices" dejavubold_32.fnt theme-fg)
        ;;create the textarea
        
        ;;create the device selector
        (glgui-box gui (* 2 padding) (vh 35) (- width (* 4 padding)) (vh 7) theme-fg)
        (set! device-selector
            (glgui-label gui 
            (* 2 padding) (vh 35) (- width (* 4 padding)) (vh 7) (get-bluetooth-device-name cursor) dejavu_16.fnt theme-bg)
        )
        (glgui-box gui (* 2 padding) (vh 25) 100 (vh 5) theme-fg)
        (glgui-button-string gui (* 2 padding) (vh 25) 100 (vh 5) "Previous" dejavu_16.fnt (lambda (un . used) 
            (set! cursor (- cursor 1))
            (glgui-widget-set! gui device-selector 'label (get-bluetooth-device-name cursor))
        ))

        (glgui-box gui (- (- width (* 2 padding)) 100) (vh 25) 100 (vh 5) theme-fg)
        (glgui-button-string gui (- (- width (* 2 padding)) 100) (vh 25) 100 (vh 5) "Next" dejavu_16.fnt (lambda (un . used) 
            (set! cursor (+ cursor 1))
            (glgui-widget-set! gui device-selector 'label (get-bluetooth-device-name cursor))
        ))

        (glgui-box gui (* 4 padding) (vh 5) (- width (* 8 padding)) (vh 10) theme-fg)
        (glgui-button-string gui (* 4 padding) (vh 5) (- width (* 8 padding)) (vh 10) "Log" dejavubold_32.fnt (lambda (un . used) 
            (glgui-widget-set! gui device-selector 'label (connect-to-bluetooth-device))
        ))
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