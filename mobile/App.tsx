import "react-native-gesture-handler";

import { NavigationContainer } from "@react-navigation/native";
import { createNativeStackNavigator } from "@react-navigation/native-stack";
import { SQLiteProvider } from "expo-sqlite";
import React from "react";
import { ActivityIndicator, View } from "react-native";

import { AuthProvider, useAuth } from "@/context/AuthContext";
import HomeScreen from "@/screens/HomeScreen";
import LoginScreen from "@/screens/LoginScreen";
import SignupScreen from "@/screens/SignupScreen";
import BiometricLoginScreen from "@/screens/BiometricLoginScreen";
import type { RootStackParamList } from "@/types/navigation";

const Stack = createNativeStackNavigator<RootStackParamList>();

function RootNavigator() {
  const { user, isLoading, biometricUserId } = useAuth();

  if (isLoading) {
    return (
      <View style={{ flex: 1, justifyContent: "center", alignItems: "center" }}>
        <ActivityIndicator size="large" />
      </View>
    );
  }

  return (
    <Stack.Navigator>
      {user ? (
        <Stack.Screen
          name="Home"
          component={HomeScreen}
          options={{ headerShown: false }}
        />
      ) : biometricUserId ? (
        <Stack.Screen
          name="BiometricLogin"
          component={BiometricLoginScreen}
          initialParams={{ userId: biometricUserId }}
          options={{ headerShown: false }}
        />
      ) : (
        <>
          <Stack.Screen
            name="Login"
            component={LoginScreen}
            options={{ title: "Login" }}
          />
          <Stack.Screen
            name="Signup"
            component={SignupScreen}
            options={{ title: "Sign Up" }}
          />
        </>
      )}
    </Stack.Navigator>
  );
}

export default function App() {
  return (
    <SQLiteProvider databaseName="app.db" useSuspense={false}>
      <AuthProvider>
        <NavigationContainer>
          <RootNavigator />
        </NavigationContainer>
      </AuthProvider>
    </SQLiteProvider>
  );
}
