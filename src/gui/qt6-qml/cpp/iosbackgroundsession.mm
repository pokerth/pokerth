/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "iosbackgroundsession.h"

#include <QtGlobal>

#ifdef Q_OS_IOS

#import <UIKit/UIKit.h>

namespace {

// Laufende Gnadenfrist (UIBackgroundTaskInvalid = keine angefordert).
UIBackgroundTaskIdentifier g_task = UIBackgroundTaskInvalid;
// Beobachter fuer Hintergrund-/Vordergrund-Wechsel; nur waehrend einer aktiven
// Online-Session registriert.
id g_didEnterBackgroundObserver = nil;
id g_willEnterForegroundObserver = nil;

// Alle UIKit-Aufrufe gehoeren auf den Main-Thread. start()/stop() koennen aus
// Netzwerk-Callbacks kommen (SignalNetClientError laeuft auf dem Netzwerk-
// Thread), deshalb konsequent dorthin verlagern - analog zum
// runOnAndroidMainThread() im Android-Pendant.
void onMainThread(void (^block)(void))
{
    if ([NSThread isMainThread])
        block();
    else
        dispatch_async(dispatch_get_main_queue(), block);
}

void endTaskIfRunning()
{
    if (g_task != UIBackgroundTaskInvalid) {
        [[UIApplication sharedApplication] endBackgroundTask:g_task];
        g_task = UIBackgroundTaskInvalid;
    }
}

void beginTask()
{
    if (g_task != UIBackgroundTaskInvalid)
        return;  // schon angefordert
    // Der expirationHandler MUSS die Task beenden. Laesst man sie auslaufen,
    // beendet iOS den Prozess hart (statt ihn nur einzufrieren).
    g_task = [[UIApplication sharedApplication]
        beginBackgroundTaskWithName:@"PokerTH server connection"
                  expirationHandler:^{
                      endTaskIfRunning();
                  }];
}

} // namespace

void IosBackgroundSession::start()
{
    onMainThread(^{
        if (g_didEnterBackgroundObserver)
            return;  // Session bereits markiert

        NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        // Wechsel in den Hintergrund: Gnadenfrist anfordern, damit ein kurzer
        // App-Wechsel den Socket nicht sofort einfrieren laesst.
        g_didEnterBackgroundObserver =
            [nc addObserverForName:UIApplicationDidEnterBackgroundNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *) { beginTask(); }];
        // Zurueck im Vordergrund: Frist sofort zurueckgeben. Nicht gehaltene
        // Zeit spart Akku, und die naechste Hintergrund-Phase bekommt wieder
        // die volle Spanne.
        g_willEnterForegroundObserver =
            [nc addObserverForName:UIApplicationWillEnterForegroundNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *) { endTaskIfRunning(); }];
    });
}

void IosBackgroundSession::stop()
{
    onMainThread(^{
        NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        if (g_didEnterBackgroundObserver) {
            [nc removeObserver:g_didEnterBackgroundObserver];
            g_didEnterBackgroundObserver = nil;
        }
        if (g_willEnterForegroundObserver) {
            [nc removeObserver:g_willEnterForegroundObserver];
            g_willEnterForegroundObserver = nil;
        }
        endTaskIfRunning();
    });
}

#endif // Q_OS_IOS
