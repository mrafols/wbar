#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include "XWin.h"
#include "ImgWrap.h"
#include "OptParser.h"
#include "SuperBar.h"
#include "Config.h"
#include "Utils.h"
#include "i18n.h"

void corpshandler(int);

int main(int argc, char **argv)
{
    #ifdef ENABLE_NLS
    setlocale( LC_ALL, "" );
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    #endif

    /* Variables */
    struct sigaction sigh;

    Bar *barra = NULL;

    try
    {
        unsigned int dblclk_tm, butpress, noreload;
        unsigned long dblclk0=0;
        int inum, vertbar;

        /* Register handler for recovering corps */
        sigh.sa_handler = corpshandler;
        sigh.sa_flags = 0;
        sigemptyset(&sigh.sa_mask); //exclude all signals
        sigaction(SIGCHLD, &sigh, NULL);

        Config config;

        std::list <App *> list = config.getAppList();
        std::list <App *>::iterator it;
        App *p;

	if (list.size() != 0)
        {
            it = list.begin();
            p = (*it);
        }
        else
        {
            throw _("Configuration empty.");
        }

        std::string command = p->getCommand();

        if (command.empty())
        {
            command = PACKAGE_NAME" "DEFAULT_ARGV;
        }

	if (argc <= 1)
        {
           std::list<std::string> list;
           Utils util;
           list = util.split ( command, " " );
           argc = list.size();

          if (argc > 1)
          {
             std::list<std::string>::iterator ac;
             argv = new char * [argc + 1];
             int i = 0;

             for (ac = list.begin();ac != list.end();ac++, i++)
             {
                argv[i] = strdup((*ac).c_str());
             }

             argv[argc] = NULL;
          }

        }

        XEvent ev;

        XWin barwin(50, 50, 50, 50);

        OptParser optparser(argc, argv);

        if (optparser.isSet(VERS))
        {
            std::cout << _("Version of ") << PACKAGE_NAME << " " << VERSION << std::endl;
            return 0;
        }
        if (optparser.isSet(HELP))
        {

	std::cout << _("Usage: wbar [option] ... [option]") << std::endl;
	std::cout << _("Options:") << std::endl;
	std::cout << "   -h, --help         " << _("this help") << std::endl;
	std::cout << "   -v, --version      " << _("show version") << std::endl;
	std::cout << "   --above-desk       " << _("run over a desktop app (ie: xfdesktop)") << std::endl;
        std::cout << "   --noreload         " << _("right click does not force reload anymore") << std::endl;
	std::cout << "   --offset i         " << _("offset bar (eg: 20)") << std::endl;
	std::cout << "   --isize  i         " << _("icon size (eg: 32)") << std::endl;
	std::cout << "   --idist  d         " << _("icon dist (eg: 1)") << std::endl;
	std::cout << "   --zoomf  z         " << _("zoom factor (eg: 1.8 or 2.5)") << std::endl;
	std::cout << "   --jumpf  j         " << _("jump factor (eg: 1.0 or 0.0)") << std::endl;
	std::cout << "   --pos    p         " << _("position:") << std::endl;
	std::cout << "                        " << "top | bottom | left | right | " << std::endl;
	std::cout << "                        " << "center | <bot|top>-<right|left>" << std::endl;
	std::cout << "   --dblclk ms        " << _("ms for double click (0: single click)") << std::endl;
	std::cout << "   --bpress           " << _("icon gets pressed") << std::endl;
	std::cout << "   --vbar             " << _("vertical bar") << std::endl;
	std::cout << "   --balfa  i         " << _("bar alfa (0-100)") << std::endl;
	std::cout << "   --falfa  i         " << _("unfocused bar alfa (0-100)") << std::endl;
	std::cout << "   --filter i         " << _("color filter (0: none 1: hovered 2: others, 3: all)") << std::endl;
	std::cout << "   --fc  0xAARRGGBB   " << _("filter color (default green 0xff00c800)") << std::endl;
	std::cout << "   --nanim  i         " << _("number of animated icons: 1, 3, 5, 7, 9, ...") << std::endl;
	std::cout << "   --nofont           " << _("if set disables font rendering") << std::endl;

#ifdef MANPAGE
	std::cout << _("View man(1).") << std::endl;
#endif
            return 0;
        }

        /* window configuration */
        if (optparser.isSet(ABOVE_DESK))
        {
            barwin.setDockWindow();
            barwin.skipTaskNPager();
            barwin.noDecorations();
            barwin.setSticky();
            barwin.bottomLayer();
        }
        else
        {
            barwin.setOverrideRedirection();
            barwin.lowerWindow();
        }

        /* tell X what events we're intrested in */
        barwin.selectInput(PointerMotionMask | ExposureMask | ButtonPressMask |
                ButtonReleaseMask | LeaveWindowMask | EnterWindowMask);

        /* Image library set up */
        INIT_IMLIB(barwin.getDisplay(), barwin.getVisual(), barwin.getColormap(),
                barwin.getDrawable(), 2048*2048);

        /* check if double clicking, ms time */
        dblclk_tm = optparser.isSet(DBLCLK)?atoi(optparser.getArg(DBLCLK).c_str()):0;
        
	butpress = optparser.isSet(BPRESS)?1:0;

	/* check if reload is admited */
    	noreload = optparser.isSet(NORELOAD)?1:0;

	vertbar = optparser.isSet(VBAR)?1:0;

        if (optparser.isSet(BALFA) || optparser.isSet(FALFA) || optparser.isSet(FILTER) || !(p->getTitle().empty() || optparser.isSet(NOFONT)))
        {
            barra = new SuperBar(&barwin, p->getIconName(), p->getTitle(),
                    optparser.isSet(ISIZE)?atoi(optparser.getArg(ISIZE).c_str()):32,
                    optparser.isSet(IDIST)?atoi(optparser.getArg(IDIST).c_str()):1,
                    optparser.isSet(ZOOMF)?atof(optparser.getArg(ZOOMF).c_str()):1.8,
                    optparser.isSet(JUMPF)?atof(optparser.getArg(JUMPF).c_str()):1,
                    vertbar, 1, optparser.isSet(NANIM)?atoi(optparser.getArg(NANIM).c_str()):5,
                    optparser.isSet(BALFA)?atoi(optparser.getArg(BALFA).c_str()):-1,
                    optparser.isSet(FALFA)?atoi(optparser.getArg(FALFA).c_str()):-1,
                    optparser.isSet(FILTER)?atoi(optparser.getArg(FILTER).c_str()):0,
                    strtoul((optparser.isSet(FC)?optparser.getArg(FC).c_str():"0xff00c800"), NULL,  16),
                    optparser.isSet(NOFONT)?0:1,
                    optparser.isSet(OFFSET)?atoi(optparser.getArg(OFFSET).c_str()):0);
        }
        else
        {
            barra = new Bar(&barwin, p->getIconName(),
                    optparser.isSet(ISIZE)?atoi(optparser.getArg(ISIZE).c_str()):32,
                    optparser.isSet(IDIST)?atoi(optparser.getArg(IDIST).c_str()):1,
                    optparser.isSet(ZOOMF)?atof(optparser.getArg(ZOOMF).c_str()):1.8,
                    optparser.isSet(JUMPF)?atof(optparser.getArg(JUMPF).c_str()):1,
                    vertbar, 1, optparser.isSet(NANIM)?atoi(optparser.getArg(NANIM).c_str()):5, 
                    optparser.isSet(OFFSET)?atoi(optparser.getArg(OFFSET).c_str()):0);
        }

        if (p) delete p;

        for (it++; it != list.end() ; it++)
        {
            p = (*it);
            ((SuperBar *)barra)->addIcon(p->getIconName(), p->getCommand(), p->getTitle());
            if (p) delete p;
        }

        /* Show the Bar */

        if (optparser.isSet(ABOVE_DESK))
        {
            barwin.mapWindow();
            barra->setPosition(optparser.getArg(POS));
        }
        else
        {
            barra->setPosition(optparser.getArg(POS));
            barwin.mapWindow();
        }

        barra->refresh();

        /* Event Loop */
        while (true)
        {

            barwin.nextEvent(&ev);
            switch (ev.type)
            {

            case Expose:
                barra->refresh();
                break;

                /* Button Press */
            case ButtonPress:
                switch (ev.xbutton.button)
                {
                case 1:
                    if (butpress!=0)
                    {
                        if (!vertbar)
                        {
                            if ((inum = barra->iconIndex(ev.xbutton.x))!=-1)
                                barra->iconDown(inum);
                        }
                        else
                        {
                            if ((inum = barra->iconIndex(ev.xbutton.y))!=-1)
                                barra->iconDown(inum);
                        }
                    }
                    break;
                case 4: //wheel up
                    //barra->setZoom(barra->getZoom()+0.1);
                    //barra->scaleIcons(ev.xbutton.x);
                    break;
                case 5:
                    //barra->setZoom(barra->getZoom()-0.1);
                    //barra->scaleIcons(ev.xbutton.x);
                    break;
                }
                break;

                /* Button Release */
            case ButtonRelease:
                switch (ev.xbutton.button)
                {
                case 3:/* Redraw Bar*/
                    if(!noreload)
			 execvp(argv[0], argv);
                    break;
                case 1:/* Execute Program */
                    if (!vertbar)
                        inum = barra->iconIndex(ev.xbutton.x);
                    else
                        inum = barra->iconIndex(ev.xbutton.y);

                    if (butpress!=0)
                        barra->iconUp(inum);

                    /* Double click time 200 ms */
                    if ((ev.xbutton.time - dblclk0 <dblclk_tm || dblclk_tm==0) && inum != -1)
                    {
                        if (fork()==0)
                        {
                            if (execlp("sh", "sh", "-c", barra->iconCommand(inum).c_str(), NULL) != 0)
                            {
                                std::cout << _("Error run program: ") << barra->iconCommand(inum) << std::endl;
                            }
                        }

                    }
                    else dblclk0 = ev.xbutton.time;
                    break;
                }
                break;
                

                /* Motion Notify */
            case MotionNotify:
                if (!vertbar)
                    barra->refresh(ev.xmotion.x);
                else
                    barra->refresh(ev.xmotion.y);
                break;
                

                /* Leave & Enter Notify */
            case LeaveNotify:
                /* NotifyGrab && Ungrab r notified on B1 click*/
                if (ev.xcrossing.mode!=NotifyGrab && !(ev.xcrossing.state&Button1Mask))
                    barra->refresh();
                break;

            case EnterNotify:
                if (ev.xcrossing.mode!=NotifyUngrab && !(ev.xcrossing.state&Button1Mask))
                {
                    if (!vertbar)
                        barra->refresh(ev.xcrossing.x);
                    else
                        barra->refresh(ev.xcrossing.y);
                }
                break;
                

            default:
                break;

            }
        }

    }
    catch (const char *m)
    {
        std::cout << m << std::endl;
    }

    if (barra) delete barra;
    return 0;
}

void corpshandler(int sig)
{
    while (waitpid(-1 ,NULL, WNOHANG)>0);
}
