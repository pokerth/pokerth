/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "iosbackgroundsession.h"

#include <QtGlobal>

#ifdef Q_OS_IOS

#import <UIKit/UIKit.h>

namespace {

// Running grace period (UIBackgroundTaskInvalid = none requested).
UIBackgroundTaskIdentifier g_task = UIBackgroundTaskInvalid;
// Observers for background/foreground changes; only registered during an
// active online session.
id g_didEnterBackgroundObserver = nil;
id g_willEnterForegroundObserver = nil;

// All UIKit calls belong on the main thread. start()/stop() can come from
// network callbacks (SignalNetClientError runs on the network thread),
// which is why they are consistently moved there - analogous to
// runOnAndroidMainThread() in the Android counterpart.
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
        return;  // already requested
    // The expirationHandler MUST end the task. If you let it run out, iOS
    // terminates the process hard (instead of merely freezing it).
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
            return;  // session already marked

        NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        // Switching to the background: request a grace period so that a short app
        // switch does not freeze the socket right away.
        g_didEnterBackgroundObserver =
            [nc addObserverForName:UIApplicationDidEnterBackgroundNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *) { beginTask(); }];
        // Back in the foreground: return the grace period immediately. Time not
        // held saves battery, and the next background phase gets the full span
        // again.
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
