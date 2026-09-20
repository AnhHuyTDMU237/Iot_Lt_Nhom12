import { useState } from 'react'
import { Box, Button, TextField, Typography, Paper } from '@mui/material'
import api from '../services/api'
import { useAuth } from '../contexts/AuthContext'

export default function Login() {
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const { login } = useAuth()

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault()
    try {
      const res = await api.post('/auth/login', { username, password })
      login(res.data.accessToken, res.data.role)
    } catch (err) {
      setError('Login failed')
    }
  }

  return (
    <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100vh', bgcolor: '#f5f5f5' }}>
      <Paper sx={{ p: 4, width: 300 }}>
        <Typography variant="h5" mb={2}>Login</Typography>
        <form onSubmit={handleLogin}>
          <TextField fullWidth label="Username" margin="normal" value={username} onChange={e => setUsername(e.target.value)} />
          <TextField fullWidth label="Password" type="password" margin="normal" value={password} onChange={e => setPassword(e.target.value)} />
          {error && <Typography color="error">{error}</Typography>}
          <Button fullWidth variant="contained" type="submit" sx={{ mt: 2 }}>Login</Button>
        </form>
      </Paper>
    </Box>
  )
}
