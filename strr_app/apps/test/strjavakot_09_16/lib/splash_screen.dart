import 'package:flutter/material.dart';
import 'wifi_connection_screen.dart';
import 'main.dart';

class SplashScreen extends StatelessWidget {
  const SplashScreen({super.key});

  // This function handles the navigation when the button is pressed.
  void _navigateToHome(BuildContext context) {
    // Navigate to the HomePage and remove the splash screen from the navigation stack,
    Navigator.pushReplacement(
      context,
      PageRouteBuilder(
        pageBuilder: (context, animation, secondaryAnimation) => const WifiConnectionScreen(),
        transitionsBuilder: (context, animation, secondaryAnimation, child) {
          return FadeTransition(opacity: animation, child: child);
        },
      ),
    );
  }


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        fit: StackFit.expand,
        children: [
          // 1. Full-screen background image
          Image.asset(
            'assets/new_icons/20250911_NEW_01_Intro.png',
            fit: BoxFit.cover,
          ),

          // 2. Main content layered on top
          SafeArea(
            child: LayoutBuilder(
              builder: (context, constraints) {
                // This LayoutBuilder gives us the actual available height
                return SingleChildScrollView(
                  child: ConstrainedBox(
                    constraints: BoxConstraints(
                      // Make the content at least as tall as the screen
                      minHeight: constraints.maxHeight,
                    ),
                    child: IntrinsicHeight( // Ensures the Column tries to be its 'natural' height
                      child: Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 24.0, vertical: 20.0),
                        child: Column(
                          children: [
                            // This spacer pushes content down from the top edge.
                            // Using SizedBox is more reliable in this structure.
                            const SizedBox(height: 40),

                            const SizedBox(height: 24),

                            const SizedBox(height: 8),

                            const Spacer(),

                            // The "Start STR" Button
                            SizedBox(height: 400),
                            _buildStartButton(context),

                            const Spacer(),

                            // SkyAutoNet Logo at the bottom
                            Image.asset('assets/icons/01_Intro_skyautonet_logo.png', height: 25),
                            const SizedBox(height: 20),
                          ],
                        ),
                      ),
                    ),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStartButton(BuildContext context) {
    return GestureDetector(
      onTap: () => _navigateToHome(context),
      child: Container(
        width: 250, // Set a fixed width for the button
        padding: const EdgeInsets.symmetric(vertical: 16),
        decoration: BoxDecoration(
          gradient: const LinearGradient(
            colors: [Color(0xFF6ECB3A), Color(0xFF3A9D29)], // Bright green gradient
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
          borderRadius: BorderRadius.circular(30), // Rounded corners
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.5),
              blurRadius: 10,
              offset: const Offset(0, 5),
            )
          ],
        ),
        child: const Center(
          child: Text(
            'Start STR',
            style: TextStyle(
              color: Colors.white,
              fontSize: 20,
              fontWeight: FontWeight.bold,
              letterSpacing: 1.1,
            ),
          ),
        ),
      ),
    );
  }
}