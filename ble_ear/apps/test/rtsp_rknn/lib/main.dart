// lib/main.dart
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:rtsp_rknn/screens/control_screen.dart';
import 'package:rtsp_rknn/state/app_state.dart';

void main() {
  // We wrap the app in a provider so the state is available everywhere.
  runApp(
    ChangeNotifierProvider(
      create: (context) => AppState(),
      child: MyAppLoader(),
    ),
  );
}

// This new widget will handle loading the settings before showing the app.
class MyAppLoader extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    // We use a consumer to get the AppState instance.
    return Consumer<AppState>(
      builder: (context, appState, child) {
        // Use a FutureBuilder to wait for the loadSettings future to complete.
        return FutureBuilder(
          future: appState.loadSettings(), // This is the async operation we wait for.
          builder: (context, snapshot) {
            // While waiting, show a simple loading indicator.
            if (snapshot.connectionState != ConnectionState.done) {
              return MaterialApp(
                home: Scaffold(
                  body: Center(child: CircularProgressIndicator()),
                ),
              );
            }

            // Once the settings are loaded, build the main app.
            return MyApp();
          },
        );
      },
    );
  }
}

// The main app widget, now built only after settings are loaded.
class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Robot Control',
      theme: ThemeData(
        primarySwatch: Colors.grey,
        brightness: Brightness.light,
      ),
      home: ControlScreen(),
    );
  }
}