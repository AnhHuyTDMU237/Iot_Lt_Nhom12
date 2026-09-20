import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom'
import Login from './pages/Login'
import Dashboard from './pages/Dashboard'
import { useAuth } from './contexts/AuthContext'
import Layout from './components/Layout'

import Logs from './pages/Logs'

function App() {
  const { token } = useAuth()

  return (
    <Router>
      <Routes>
        <Route path="/login" element={token ? <Navigate to="/" /> : <Login />} />
        <Route path="/" element={token ? <Layout /> : <Navigate to="/login" />}>
          <Route index element={<Dashboard />} />
          <Route path="logs" element={<Logs />} />
        </Route>
      </Routes>
    </Router>
  )
}

export default App
