/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/

package org.pokerth.qml;

import android.Manifest;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;

/**
 * Foreground service that is active while an online session (lobby or game)
 * is running. Its only purpose is to keep the process alive and network
 * access allowed while the app is in the background: without it, Android
 * (Doze / app freezer / vendor battery managers) silently kills the TCP
 * connection to the game server after a while, and the player loses the
 * table seat. Started/stopped from C++ via JNI (androidconnectionservice.cpp)
 * on client start / disconnect.
 *
 * The notification texts are passed in from C++ so they use the Qt
 * translations of the app language.
 *
 * While the service runs it also holds a partial wake lock and a WiFi lock:
 * the foreground state protects the process from the app freezer, but it does
 * not stop the CPU from suspending or the WiFi radio from entering power save
 * once the screen goes off - which drops the connection just the same.
 */
public class ConnectionService extends Service {
    private static final String CHANNEL_ID = "pokerth_connection";
    private static final int NOTIFICATION_ID = 0x504b5448; // "PKTH"

    private static final String EXTRA_TEXT = "notificationText";
    private static final String EXTRA_CHANNEL_NAME = "channelName";

    private static final String WAKE_LOCK_TAG = "PokerTH:connection";
    private static final int NOTIFICATION_PERMISSION_REQUEST = 0x504b;

    // Ask for the notification permission at most once per process: Android
    // itself stops showing the dialog after two denials, but re-firing the
    // request on every reconnect would be pointless churn.
    private static boolean notificationPermissionAsked = false;

    private PowerManager.WakeLock wakeLock;
    private WifiManager.WifiLock wifiLock;

    public static void start(Context context, String notificationText, String channelName) {
        ensureNotificationPermission(context);
        Intent intent = new Intent(context, ConnectionService.class);
        intent.putExtra(EXTRA_TEXT, notificationText);
        intent.putExtra(EXTRA_CHANNEL_NAME, channelName);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            context.startForegroundService(intent);
        else
            context.startService(intent);
    }

    /**
     * API 33+: POST_NOTIFICATIONS is a runtime permission. Without it the
     * foreground service still runs, but its notification stays invisible -
     * the player gets no way back to the table and no hint why the app keeps
     * running. Asking here means the dialog appears exactly when the session
     * starts, which is the moment it can be explained by the context.
     *
     * Fire-and-forget: the service is started regardless of the answer, so
     * there is no result callback to handle.
     */
    private static void ensureNotificationPermission(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
            return;
        if (notificationPermissionAsked)
            return;
        if (context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                == PackageManager.PERMISSION_GRANTED)
            return;
        // Only an Activity can request permissions. Qt hands us the QtActivity
        // as context; guard anyway so a plain app context cannot crash us.
        if (!(context instanceof Activity))
            return;
        notificationPermissionAsked = true;
        ((Activity) context).requestPermissions(
            new String[] { Manifest.permission.POST_NOTIFICATIONS },
            NOTIFICATION_PERMISSION_REQUEST);
    }

    public static void stop(Context context) {
        context.stopService(new Intent(context, ConnectionService.class));
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String text = null;
        String channelName = null;
        if (intent != null) {
            text = intent.getStringExtra(EXTRA_TEXT);
            channelName = intent.getStringExtra(EXTRA_CHANNEL_NAME);
        }
        if (text == null)
            text = "Connected to the game server";
        if (channelName == null)
            channelName = "Online game connection";

        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID, channelName, NotificationManager.IMPORTANCE_LOW);
            channel.setShowBadge(false);
            NotificationManager nm =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            nm.createNotificationChannel(channel);
            builder = new Notification.Builder(this, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(this);
        }

        builder.setContentTitle("PokerTH")
               .setContentText(text)
               .setSmallIcon(getApplicationInfo().icon)
               .setOngoing(true);

        // Tapping the notification returns to the app.
        Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
        if (launch != null) {
            builder.setContentIntent(PendingIntent.getActivity(
                this, 0, launch,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE));
        }

        Notification notification = builder.build();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            // API 34+: the service type is mandatory at startForeground time.
            startForeground(NOTIFICATION_ID, notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }

        acquireLocks();

        // If Android ever kills the service, do not restart it without the app:
        // the connection it protects is gone anyway.
        return START_NOT_STICKY;
    }

    /**
     * Keep the CPU and the WiFi radio awake for as long as the session lasts.
     * Both locks are strictly bound to the service lifetime, so the battery
     * cost only applies while the player actually sits at a table.
     *
     * onStartCommand may run more than once (start() on an already running
     * service), hence the isHeld() guards - the locks are not reference
     * counted, so a second acquire would otherwise leak.
     */
    private void acquireLocks() {
        if (wakeLock == null) {
            PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
            if (pm != null) {
                wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, WAKE_LOCK_TAG);
                wakeLock.setReferenceCounted(false);
            }
        }
        if (wakeLock != null && !wakeLock.isHeld()) {
            // No timeout: the lock is released in onDestroy, and if the
            // process dies Android drops it with the process.
            wakeLock.acquire();
        }

        if (wifiLock == null) {
            WifiManager wm = (WifiManager)
                getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wm != null) {
                // WIFI_MODE_FULL_HIGH_PERF is deprecated since API 29 but is
                // the only mode that still keeps the radio out of power save
                // while the app is in the background: WIFI_MODE_FULL became a
                // no-op, and WIFI_MODE_FULL_LOW_LATENCY only takes effect
                // while the app is in the foreground with the screen on -
                // exactly the case we do not need to protect.
                wifiLock = wm.createWifiLock(
                    WifiManager.WIFI_MODE_FULL_HIGH_PERF, WAKE_LOCK_TAG);
                wifiLock.setReferenceCounted(false);
            }
        }
        if (wifiLock != null && !wifiLock.isHeld())
            wifiLock.acquire();
    }

    private void releaseLocks() {
        if (wifiLock != null && wifiLock.isHeld())
            wifiLock.release();
        if (wakeLock != null && wakeLock.isHeld())
            wakeLock.release();
    }

    @Override
    public void onDestroy() {
        releaseLocks();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
