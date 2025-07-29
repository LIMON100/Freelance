// lib/ahrs.dart (Corrected and Robust Version)

import 'dart:math';
import 'package:vector_math/vector_math.dart';

class MadgwickAHRS {
  double samplePeriod;
  double beta; // Algorithm gain
  Quaternion q = Quaternion(1.0, 0.0, 0.0, 0.0);

  MadgwickAHRS({required this.samplePeriod, required this.beta});

  void updateIMU(Vector3 gyroscope, Vector3 accelerometer) {
    // Local copy of quaternion
    double q0 = q.w, q1 = q.x, q2 = q.y, q3 = q.z;

    // Convert gyroscope from degrees/sec to radians/sec
    double gx = radians(gyroscope.x);
    double gy = radians(gyroscope.y);
    double gz = radians(gyroscope.z);

    // Local accelerometer values
    double ax = accelerometer.x;
    double ay = accelerometer.y;
    double az = accelerometer.z;

    // Rate of change of quaternion from gyroscope
    double qDot1, qDot2, qDot3, qDot4;
    qDot1 = 0.5 * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5 * (q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5 * (q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5 * (q0 * gz + q1 * gy - q2 * gx);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if (!((ax == 0.0) && (ay == 0.0) && (az == 0.0))) {
      // Normalise accelerometer measurement
      double recipNorm = 1.0 / sqrt(ax * ax + ay * ay + az * az);
      ax *= recipNorm;
      ay *= recipNorm;
      az *= recipNorm;

      // Auxiliary variables to avoid repeated arithmetic
      double s0, s1, s2, s3;
      double _2q0 = 2.0 * q0;
      double _2q1 = 2.0 * q1;
      double _2q2 = 2.0 * q2;
      double _2q3 = 2.0 * q3;
      double _4q0 = 4.0 * q0;
      double _4q1 = 4.0 * q1;
      double _4q2 = 4.0 * q2;
      double _8q1 = 8.0 * q1;
      double _8q2 = 8.0 * q2;
      double q0q0 = q0 * q0;
      double q1q1 = q1 * q1;
      double q2q2 = q2 * q2;
      double q3q3 = q3 * q3;

      // Gradient decent algorithm corrective step
      s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
      s1 = _4q1 * q3q3 - _2q3 * ax + 4.0 * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
      s2 = 4.0 * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
      s3 = 4.0 * q1q1 * q3 - _2q1 * ax + 4.0 * q2q2 * q3 - _2q2 * ay;

      recipNorm = 1.0 / sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
      s0 *= recipNorm;
      s1 *= recipNorm;
      s2 *= recipNorm;
      s3 *= recipNorm;

      // Apply feedback step
      qDot1 -= beta * s0;
      qDot2 -= beta * s1;
      qDot3 -= beta * s2;
      qDot4 -= beta * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    q0 += qDot1 * samplePeriod;
    q1 += qDot2 * samplePeriod;
    q2 += qDot3 * samplePeriod;
    q3 += qDot4 * samplePeriod;

    // Normalise quaternion
    double recipNorm = 1.0 / sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;

    // Update the class quaternion
    q.setValues(q1, q2, q3, q0);
  }

  /// Sets the current heading (Yaw) of the AHRS algorithm.
  void setHeading(double headingDegrees) {
    final euler = toEulerAngles(); // Current roll, pitch, yaw
    final halfYaw = radians(headingDegrees) / 2.0;
    final halfRoll = radians(euler.x) / 2.0;
    final halfPitch = radians(euler.y) / 2.0;

    q.w = cos(halfRoll) * cos(halfPitch) * cos(halfYaw) + sin(halfRoll) * sin(halfPitch) * sin(halfYaw);
    q.x = sin(halfRoll) * cos(halfPitch) * cos(halfYaw) - cos(halfRoll) * sin(halfPitch) * sin(halfYaw);
    q.y = cos(halfRoll) * sin(halfPitch) * cos(halfYaw) + sin(halfRoll) * cos(halfPitch) * sin(halfYaw);
    q.z = cos(halfRoll) * cos(halfPitch) * sin(halfYaw) - sin(halfRoll) * sin(halfPitch) * cos(halfYaw);
    q.normalize();
  }

  /// Converts the quaternion output to Euler angles (in degrees).
  Vector3 toEulerAngles() {
    final w = q.w, x = q.x, y = q.y, z = q.z;
    final sinr_cosp = 2 * (w * x + y * z);
    final cosr_cosp = 1 - 2 * (x * x + y * y);
    final roll = atan2(sinr_cosp, cosr_cosp);

    final sinp = 2 * (w * y - z * x);
    final pitch = (sinp.abs() >= 1) ? (pi / 2 * sinp.sign) : asin(sinp);

    final siny_cosp = 2 * (w * z + x * y);
    final cosy_cosp = 1 - 2 * (y * y + z * z);
    final yaw = atan2(siny_cosp, cosy_cosp);

    return Vector3(degrees(roll), degrees(pitch), degrees(yaw));
  }
}