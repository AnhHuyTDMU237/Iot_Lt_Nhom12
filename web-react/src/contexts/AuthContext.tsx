import { createContext, useContext, useState, ReactNode } from 'react'

interface AuthContextType {
  token: String | null
  role: String | null
  login: (token: string, role: string) => void
  logout: () => void
}

const AuthContext = createContext<AuthContextType>(null!)

export const AuthProvider = ({ children }: { children: ReactNode }) => {
  const [token, setToken] = useState<String | null>(localStorage.getItem('token'))
  const [role, setRole] = useState<String | null>(localStorage.getItem('role'))

  const login = (newToken: string, newRole: string) => {
    setToken(newToken)
    setRole(newRole)
    localStorage.setItem('token', newToken)
    localStorage.setItem('role', newRole)
  }

  const logout = () => {
    setToken(null)
    setRole(null)
    localStorage.removeItem('token')
    localStorage.removeItem('role')
  }

  return (
    <AuthContext.Provider value={{ token, role, login, logout }}>
      {children}
    </AuthContext.Provider>
  )
}

export const useAuth = () => useContext(AuthContext)
