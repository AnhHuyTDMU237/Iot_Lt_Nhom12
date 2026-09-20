import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';

class ApiService extends ChangeNotifier {
  // Use 10.0.2.2 for Android emulator to access host localhost
  final String baseUrl = 'http://10.0.2.2:8080/api/v1';
  String? _token;
  String? _role;

  bool get isAuthenticated => _token != null;
  String? get role => _role;

  ApiService() {
    _loadToken();
  }

  Future<void> _loadToken() async {
    final prefs = await SharedPreferences.getInstance();
    _token = prefs.getString('token');
    _role = prefs.getString('role');
    notifyListeners();
  }

  Future<bool> login(String username, String password) async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/auth/login'),
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({'username': username, 'password': password}),
      );

      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        _token = data['accessToken'];
        _role = data['role'];
        final prefs = await SharedPreferences.getInstance();
        await prefs.setString('token', _token!);
        await prefs.setString('role', _role!);
        notifyListeners();
        return true;
      }
    } catch (e) {
      debugPrint(e.toString());
    }
    return false;
  }

  void logout() async {
    _token = null;
    _role = null;
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove('token');
    await prefs.remove('role');
    notifyListeners();
  }

  Future<Map<String, dynamic>?> getDevice(String deviceId) async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/devices/$deviceId'),
        headers: {'Authorization': 'Bearer $_token'},
      );
      if (response.statusCode == 200) {
        return jsonDecode(response.body);
      }
    } catch (e) {
      debugPrint(e.toString());
    }
    return null;
  }

  Future<Map<String, dynamic>?> getLatestTelemetry(String deviceId) async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/devices/$deviceId/telemetry/latest'),
        headers: {'Authorization': 'Bearer $_token'},
      );
      if (response.statusCode == 200) {
        return jsonDecode(response.body);
      }
    } catch (e) {
      debugPrint(e.toString());
    }
    return null;
  }

  Future<bool> sendCommand(String deviceId, String action) async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/devices/$deviceId/commands'),
        headers: {
          'Authorization': 'Bearer $_token',
          'Content-Type': 'application/json'
        },
        body: jsonEncode({'action': action}),
      );
      return response.statusCode == 200;
    } catch (e) {
      debugPrint(e.toString());
    }
    return false;
  }
}
