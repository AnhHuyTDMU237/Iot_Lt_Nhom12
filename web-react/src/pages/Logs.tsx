import { useEffect, useState } from 'react'
import { Paper, Typography, Table, TableBody, TableCell, TableContainer, TableHead, TableRow, TablePagination, Box } from '@mui/material'
import api from '../services/api'
import { format } from 'date-fns'

interface Telemetry {
  id: string
  temperature: number
  humidity: number
  illuminance: number
  soilMoisture: number
  recordedAt: string
}

export default function Logs() {
  const [logs, setLogs] = useState<Telemetry[]>([])
  const [page, setPage] = useState(0)
  const [total, setTotal] = useState(0)
  const rowsPerPage = 10

  const fetchLogs = async () => {
    try {
      const res = await api.get(`/devices/esp32-001/telemetry?page=${page}&size=${rowsPerPage}`)
      setLogs(res.data.content)
      setTotal(res.data.totalElements)
    } catch (err) {
      console.error(err)
    }
  }

  useEffect(() => {
    fetchLogs()
    const interval = setInterval(fetchLogs, 10000) // Poll every 10s
    return () => clearInterval(interval)
  }, [page])

  return (
    <Box>
      <Typography variant="h4" mb={3} fontWeight="bold">History Logs</Typography>
      <Paper sx={{ width: '100%', overflow: 'hidden', borderRadius: 2, boxShadow: 3 }}>
        <TableContainer sx={{ maxHeight: 600 }}>
          <Table stickyHeader aria-label="sticky table">
            <TableHead>
              <TableRow>
                <TableCell sx={{ fontWeight: 'bold' }}>Time</TableCell>
                <TableCell sx={{ fontWeight: 'bold' }} align="right">Temperature (°C)</TableCell>
                <TableCell sx={{ fontWeight: 'bold' }} align="right">Humidity (%)</TableCell>
                <TableCell sx={{ fontWeight: 'bold' }} align="right">Light (lux)</TableCell>
                <TableCell sx={{ fontWeight: 'bold' }} align="right">Soil Moisture (%)</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {logs.map((row) => (
                <TableRow hover role="checkbox" tabIndex={-1} key={row.id}>
                  <TableCell>{format(new Date(row.recordedAt), 'yyyy-MM-dd HH:mm:ss')}</TableCell>
                  <TableCell align="right">{row.temperature}</TableCell>
                  <TableCell align="right">{row.humidity}</TableCell>
                  <TableCell align="right">{row.illuminance || '--'}</TableCell>
                  <TableCell align="right">{row.soilMoisture || '--'}</TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
        <TablePagination
          rowsPerPageOptions={[10]}
          component="div"
          count={total}
          rowsPerPage={rowsPerPage}
          page={page}
          onPageChange={(_, newPage) => setPage(newPage)}
        />
      </Paper>
    </Box>
  )
}
