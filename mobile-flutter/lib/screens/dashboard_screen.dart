import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/api_service.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  Map<String, dynamic>? _device;
  Map<String, dynamic>? _telemetry;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _fetchData();
    _timer = Timer.periodic(const Duration(seconds: 5), (_) => _fetchData());
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  Future<void> _fetchData() async {
    final api = context.read<ApiService>();
    final device = await api.getDevice('esp32-001');
    final telemetry = await api.getLatestTelemetry('esp32-001');
    if (mounted) {
      setState(() {
        _device = device;
        _telemetry = telemetry;
      });
    }
  }

  void _toggleLed() async {
    if (_device == null) return;
    final api = context.read<ApiService>();
    final action = _device!['ledState'] == true ? 'LED_OFF' : 'LED_ON';
    await api.sendCommand('esp32-001', action);
    _fetchData();
  }

  @override
  Widget build(BuildContext context) {
    final api = context.watch<ApiService>();
    
    return Scaffold(
      appBar: AppBar(
        title: Text('Dashboard (${api.role})'),
        actions: [
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () => api.logout(),
          ),
        ],
      ),
      body: _device == null
          ? const Center(child: CircularProgressIndicator())
          : Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Device: ${_device!['name']}', style: const TextStyle(fontSize: 20)),
                  Text('Status: ${_device!['status']}', 
                    style: TextStyle(
                      color: _device!['status'] == 'ONLINE' ? Colors.green : Colors.red,
                      fontWeight: FontWeight.bold
                    )),
                  const SizedBox(height: 20),
                  if (_telemetry != null) ...[
                    Text('Temperature: ${_telemetry!['temperature']} °C'),
                    Text('Humidity: ${_telemetry!['humidity']} %'),
                  ],
                  const SizedBox(height: 20),
                  if (api.role != 'VIEWER')
                    Row(
                      children: [
                        const Text('LED Control: '),
                        Switch(
                          value: _device!['ledState'] ?? false,
                          onChanged: (_) => _toggleLed(),
                        ),
                      ],
                    ),
                  if (api.role == 'VIEWER')
                    Text('LED is ${_device!['ledState'] == true ? 'ON' : 'OFF'}'),
                ],
              ),
            ),
    );
  }
}
