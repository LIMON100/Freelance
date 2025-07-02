import 'package:flutter/material.dart';
import 'wifi_connection_screen.dart';
import 'main.dart';

class SplashScreen extends StatelessWidget {
  const SplashScreen({super.key});

  // This function handles the navigation when the button is pressed.
  void _navigateToHome(BuildContext context) {
    // Navigate to the HomePage and remove the splash screen from the navigation stack,
    // so the user can't go back to it.
    Navigator.pushReplacement(
      context,
      // We use a FadeTransition for a smoother screen change
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
            'assets/icons/01_Intro_BG.png', // Your camouflage background
            fit: BoxFit.cover,
          ),

          // 2. Main content layered on top
          SafeArea(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24.0, vertical: 40.0),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  // This spacer pushes the content down from the top edge
                  const Spacer(flex: 3),

                  // Emblem at the top
                  Image.asset('assets/icons/01_Intro_logo.png', height: 100), // Adjust size if needed
                  const SizedBox(height: 24),

                  // Main STR Logo
                  Image.asset('assets/icons/01_Intro_str_logo.png', width: 280), // Adjust size if needed
                  const SizedBox(height: 8),

                  // Subtitle Text
                  const Text(
                    'Small Tactical Robot',
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 22,
                      fontWeight: FontWeight.w300,
                      letterSpacing: 1.5,
                    ),
                  ),

                  // This spacer creates the large gap above the button
                  const Spacer(flex: 4),

                  // The "Start STR" Button
                  _buildStartButton(context),

                  // This spacer pushes the bottom logo down
                  const Spacer(flex: 2),

                  // SkyAutoNet Logo at the bottom
                  Image.asset('assets/icons/01_Intro_skyautonet_logo.png', height: 25), // Adjust size if needed
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  /// A helper method to create the styled "Start STR" button.
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