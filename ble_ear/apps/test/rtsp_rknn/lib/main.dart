// lib/main.dart
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:rtsp_rknn/screens/control_screen.dart';
import 'package:rtsp_rknn/state/app_state.dart';

void main() {
  runApp(
    ChangeNotifierProvider(
      create: (context) => AppState(),
      child: MyApp(),
    ),
  );
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Robot Control',
      theme: ThemeData(
        primarySwatch: Colors.grey,
        brightness: Brightness.light, // The UI looks dark
      ),
      home: ControlScreen(),
    );
  }
}