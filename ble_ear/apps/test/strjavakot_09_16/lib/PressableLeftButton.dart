import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

class PressableLeftButton extends StatefulWidget {
  final double left;
  final double top;
  final String label;
  final String inactiveIcon;
  final String activeIcon;
  final bool isActive;
  final VoidCallback onPressed;

  const PressableLeftButton({
    Key? key,
    required this.left,
    required this.top,
    required this.label,
    required this.inactiveIcon,
    required this.activeIcon,
    required this.isActive,
    required this.onPressed,
  }) : super(key: key);

  @override
  _PressableLeftButtonState createState() => _PressableLeftButtonState();
}

class _PressableLeftButtonState extends State<PressableLeftButton> {
  bool _isPressed = false;

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    // Use a solid red color when pressed, otherwise black
    final containerColor = _isPressed ? Color(0xFFc32121) : Colors.black.withOpacity(0.6);

    return Positioned(
      left: widget.left * widthScale,
      top: widget.top * heightScale,
      child: GestureDetector(
        onTapDown: (_) => setState(() => _isPressed = true),
        onTapUp: (_) {
          setState(() => _isPressed = false);
          widget.onPressed();
        },
        onTapCancel: () => setState(() => _isPressed = false),
        child: Container(
          width: 200 * widthScale,
          height: 120 * heightScale,
          padding: EdgeInsets.symmetric(vertical: 8.0 * heightScale),
          decoration: BoxDecoration(
            color: containerColor, // <-- THE IMPORTANT CHANGE
            borderRadius: BorderRadius.circular(10),
            border: Border.all(
              color: widget.isActive ? Colors.white : Colors.transparent,
              width: 2.0,
            ),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(
                flex: 2,
                child: Image.asset(
                  widget.isActive ? widget.activeIcon : widget.inactiveIcon,
                  fit: BoxFit.contain,
                ),
              ),
              SizedBox(height: 5 * heightScale),
              Text(
                widget.label,
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.bold,
                  fontSize: 24 * heightScale,
                  color: Colors.white,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}