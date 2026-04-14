import React, { useMemo } from 'react';
import { ActivityIndicator, View } from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';
import { NavigationContainer, DarkTheme } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import { StatusBar } from 'expo-status-bar';
import { SQLiteProvider } from 'expo-sqlite';

import { AuthProvider, useAuth } from '@/context/AuthContext';
import { initDB } from '@/db/database';
import LoginScreen from '@/screens/LoginScreen';
import SignupScreen from '@/screens/SignupScreen';
import DashboardScreen from './app/(tabs)/index';
import RemoteScreen from './app/(tabs)/remote';
import IntelligenceScreen from './app/(tabs)/intelligence';
import SensorsScreen from './app/(tabs)/sensors';
import SettingsScreen from './app/modal-settings';

const AuthStack = createNativeStackNavigator();
const AppStack = createNativeStackNavigator();
const Tabs = createBottomTabNavigator();

function AuthNavigator() {
  return (
    <AuthStack.Navigator screenOptions={{ headerShown: false }}>
      <AuthStack.Screen name="Login" component={LoginScreen} />
      <AuthStack.Screen name="Signup" component={SignupScreen} />
    </AuthStack.Navigator>
  );
}

function AppTabs() {
  return (
    <Tabs.Navigator
      screenOptions={{
        headerShown: false,
        tabBarActiveTintColor: '#72F88A',
        tabBarInactiveTintColor: '#6C7473',
        tabBarStyle: {
          backgroundColor: '#1B1F1C',
          borderTopWidth: 0,
          marginHorizontal: 12,
          marginBottom: 10,
          borderRadius: 22,
          height: 74,
          position: 'absolute',
          paddingTop: 8,
          paddingBottom: 10,
        },
        tabBarLabelStyle: {
          fontSize: 11,
          fontWeight: '700',
          letterSpacing: 1.1,
        },
      }}
    >
      <Tabs.Screen
        name="Mission"
        component={DashboardScreen}
        options={{
          title: 'MISSION',
          tabBarIcon: ({ color, size }) => (
            <MaterialCommunityIcons name="view-grid" color={color} size={size} />
          ),
        }}
      />
      <Tabs.Screen
        name="Remote"
        component={RemoteScreen}
        options={{
          title: 'REMOTE',
          tabBarIcon: ({ color, size }) => (
            <MaterialCommunityIcons name="account-cog-outline" color={color} size={size} />
          ),
        }}
      />
      <Tabs.Screen
        name="Intelligence"
        component={IntelligenceScreen}
        options={{
          title: 'INTELLIGENCE',
          tabBarIcon: ({ color, size }) => (
            <MaterialCommunityIcons name="brain" color={color} size={size} />
          ),
        }}
      />
      <Tabs.Screen
        name="Sensors"
        component={SensorsScreen}
        options={{
          title: 'SENSORS',
          tabBarIcon: ({ color, size }) => (
            <MaterialCommunityIcons name="access-point" color={color} size={size} />
          ),
        }}
      />
      <Tabs.Screen
        name="Settings"
        component={SettingsScreen}
        options={{
          title: 'SETTINGS',
          tabBarIcon: ({ color, size }) => (
            <MaterialCommunityIcons name="cog" color={color} size={size} />
          ),
        }}
      />
    </Tabs.Navigator>
  );
}

function AppNavigator() {
  const { user, isLoading } = useAuth();

  const loadingView = useMemo(
    () => (
      <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center', backgroundColor: '#070A0A' }}>
        <ActivityIndicator size="large" color="#72F88A" />
      </View>
    ),
    []
  );

  if (isLoading) {
    return loadingView;
  }

  return (
    <NavigationContainer theme={DarkTheme}>
      {user ? <AppTabs /> : <AuthNavigator />}
    </NavigationContainer>
  );
}

export default function App() {
  return (
    <GestureHandlerRootView style={{ flex: 1 }}>
      <StatusBar style="light" />
      <SQLiteProvider databaseName="app.db" onInit={initDB}>
        <AuthProvider>
          <AppNavigator />
        </AuthProvider>
      </SQLiteProvider>
    </GestureHandlerRootView>
  );
}
