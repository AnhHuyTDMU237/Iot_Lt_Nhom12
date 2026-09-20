import { useEffect, useState } from 'react'
import { Card, Typography, Grid, Switch, FormControlLabel, Paper, Box } from '@mui/material'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Legend } from 'recharts'
import DeviceThermostatIcon from '@mui/icons-material/DeviceThermostat'
import WaterDropIcon from '@mui/icons-material/WaterDrop'
import WbSunnyIcon from '@mui/icons-material/WbSunny'
import GrassIcon from '@mui/icons-material/Grass'
import api from '../services/api'
import { useAuth } from '../contexts/AuthContext'
import { format } from 'date-fns'

interface Device {
  deviceId: string
  name: string
  status: string
  ledState: boolean
}

interface Telemetry {
  temperature: number
  humidity: number
  illuminance: number
  soilMoisture: number
  recordedAt: string
}

const MetricCard = ({ title, value, unit, icon, color }: any) => (
  <Card sx={{ height: '100%', display: 'flex', alignItems: 'center', p: 2, borderRadius: 2, boxShadow: 3 }}>
    <Box sx={{ p: 2, borderRadius: '50%', bgcolor: `${color}.light`, color: `${color}.main`, mr: 2 }}>
      {icon}
    </Box>
    <Box>
      <Typography variant="body2" color="textSecondary">{title}</Typography>
      <Typography variant="h5" sx={{ fontWeight: 'bold' }}>{value !== undefined ? `${value} ${unit}` : '--'}</Typography>
    </Box>
  </Card>
)

export default function Dashboard() {
  const [device, setDevice] = useState<Device | null>(null)
  const [history, setHistory] = useState<Telemetry[]>([])
  const { role } = useAuth()

  const fetchData = async () => {
    try {
      const devRes = await api.get('/devices/esp32-001')
      setDevice(devRes.data)
      const histRes = await api.get('/devices/esp32-001/telemetry?size=20')
      setHistory(histRes.data.content.reverse())
    } catch (err) {
      console.error(err)
    }
  }

  useEffect(() => {
    fetchData()
    const interval = setInterval(fetchData, 5000)
    return () => clearInterval(interval)
  }, [])

  const toggleLed = async () => {
    if (!device) return
    const action = device.ledState ? 'LED_OFF' : 'LED_ON'
    try {
      await api.post(`/devices/${device.deviceId}/commands`, { action })
      setDevice({ ...device, ledState: !device.ledState })
    } catch (err) {
      console.error(err)
    }
  }

  if (!device) return <Typography>Loading dashboard...</Typography>

  const chartData = history.map(t => ({
    time: format(new Date(t.recordedAt), 'HH:mm:ss'),
    temperature: t.temperature,
    humidity: t.humidity,
    illuminance: t.illuminance,
    soilMoisture: t.soilMoisture
  }))

  const latest = history[history.length - 1] || {} as any

  return (
    <Box>
      <Typography variant="h4" mb={3} fontWeight="bold">Environment Overview</Typography>
      
      {/* Metrics Row */}
      <Grid container spacing={3} mb={4}>
        <Grid item xs={12} sm={6} md={3}>
          <MetricCard title="Temperature" value={latest.temperature} unit="°C" icon={<DeviceThermostatIcon fontSize="large" />} color="error" />
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <MetricCard title="Humidity" value={latest.humidity} unit="%" icon={<WaterDropIcon fontSize="large" />} color="info" />
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <MetricCard title="Light Level" value={latest.illuminance} unit="lux" icon={<WbSunnyIcon fontSize="large" />} color="warning" />
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <MetricCard title="Soil Moisture" value={latest.soilMoisture} unit="%" icon={<GrassIcon fontSize="large" />} color="success" />
        </Grid>
      </Grid>

      <Grid container spacing={3}>
        {/* Charts */}
        <Grid item xs={12} lg={8}>
          <Paper sx={{ p: 3, borderRadius: 2, boxShadow: 3 }}>
            <Typography variant="h6" mb={2} fontWeight="bold">Real-time Telemetry</Typography>
            <div style={{ height: 350 }}>
              <ResponsiveContainer width="100%" height="100%">
                <LineChart data={chartData}>
                  <CartesianGrid strokeDasharray="3 3" vertical={false} />
                  <XAxis dataKey="time" />
                  <YAxis yAxisId="left" />
                  <YAxis yAxisId="right" orientation="right" />
                  <Tooltip contentStyle={{ borderRadius: 8 }} />
                  <Legend />
                  <Line yAxisId="left" type="monotone" dataKey="temperature" stroke="#d32f2f" name="Temp (°C)" strokeWidth={2} dot={false} />
                  <Line yAxisId="left" type="monotone" dataKey="humidity" stroke="#0288d1" name="Humidity (%)" strokeWidth={2} dot={false} />
                  <Line yAxisId="right" type="monotone" dataKey="soilMoisture" stroke="#2e7d32" name="Soil (%)" strokeWidth={2} dot={false} />
                  <Line yAxisId="right" type="monotone" dataKey="illuminance" stroke="#ed6c02" name="Light (lux)" strokeWidth={2} dot={false} />
                </LineChart>
              </ResponsiveContainer>
            </div>
          </Paper>
        </Grid>

        {/* Device Control */}
        <Grid item xs={12} lg={4}>
          <Paper sx={{ p: 3, borderRadius: 2, boxShadow: 3, height: '100%' }}>
            <Typography variant="h6" mb={2} fontWeight="bold">Device Control</Typography>
            <Box sx={{ mb: 2 }}>
              <Typography variant="subtitle2" color="textSecondary">Device Name</Typography>
              <Typography variant="body1">{device.name}</Typography>
            </Box>
            <Box sx={{ mb: 3 }}>
              <Typography variant="subtitle2" color="textSecondary">Connection Status</Typography>
              <Typography 
                variant="body1" 
                sx={{ 
                  color: device.status === 'ONLINE' ? 'success.main' : 'error.main',
                  fontWeight: 'bold'
                }}
              >
                {device.status}
              </Typography>
            </Box>
            
            <Box sx={{ p: 2, bgcolor: '#f5f5f5', borderRadius: 2 }}>
              <Typography variant="subtitle2" mb={1}>Actuators</Typography>
              {role !== 'VIEWER' ? (
                <FormControlLabel
                  control={<Switch checked={device.ledState} onChange={toggleLed} color="primary" />}
                  label={`Main LED / Relay: ${device.ledState ? 'ON' : 'OFF'}`}
                />
              ) : (
                <Typography color="textSecondary">
                  Main LED / Relay: <b>{device.ledState ? 'ON' : 'OFF'}</b> (Read-only)
                </Typography>
              )}
            </Box>
          </Paper>
        </Grid>
      </Grid>
    </Box>
  )
}
