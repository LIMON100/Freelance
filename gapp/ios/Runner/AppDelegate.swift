import Flutter
import UIKit
import FirebaseCore
import flutter_local_notifications
import GoogleMaps

@main
@objc class AppDelegate: FlutterAppDelegate {
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
  GMSServices.provideAPIKey("AIzaSyBUuMcDqSCZTtx9HGA-pSH61Fet3hWwccQ")
   if FirebaseApp.app()==nil{
            FirebaseApp.configure()
        }
        FlutterLocalNotificationsPlugin.setPluginRegistrantCallback { (registry) in
               GeneratedPluginRegistrant.register(with: registry)
           }

           if #available(iOS 10.0, *) {
             UNUserNotificationCenter.current().delegate = self as UNUserNotificationCenterDelegate
           }
        GeneratedPluginRegistrant.register(with: self)
    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }
}
