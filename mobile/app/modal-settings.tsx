import React, { useState } from 'react';
import {
  SafeAreaView,
  ScrollView,
  View,
  Text,
  Pressable,
  Switch,
  StyleSheet,
} from 'react-native';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import { useRouter } from 'expo-router';

const settingsStyles = StyleSheet.create({
  safe: {
    flex: 1,
    backgroundColor: '#070A0A',
  },
  container: {
    flex: 1,
    paddingHorizontal: 16,
    paddingVertical: 12,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 24,
  } as any,
  backButton: {
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: '#151718',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  } as any,
  headerTitle: {
    color: '#FFFFFF',
    fontSize: 28,
    fontWeight: '700' as any,
  },
  section: {
    marginBottom: 24,
  },
  sectionTitle: {
    color: '#7A8582',
    fontSize: 12,
    fontWeight: '700' as any,
    letterSpacing: 0.5,
    marginBottom: 12,
    textTransform: 'uppercase' as any,
  },
  settingItem: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: '#151718',
    paddingHorizontal: 16,
    paddingVertical: 14,
    borderRadius: 12,
    marginBottom: 8,
  } as any,
  settingLabel: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600' as any,
  },
  settingDescription: {
    color: '#7A8582',
    fontSize: 12,
    marginTop: 4,
  },
  divider: {
    height: 1,
    backgroundColor: '#0C0E0F',
    marginVertical: 16,
  },
  dangerButton: {
    backgroundColor: '#E74C3C',
    borderRadius: 12,
    paddingHorizontal: 16,
    paddingVertical: 14,
    alignItems: 'center',
  } as any,
  dangerButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700' as any,
    letterSpacing: 0.3,
  },
});

export default function SettingsScreen() {
  const router = useRouter();
  const [notifications, setNotifications] = useState(true);
  const [autoConnect, setAutoConnect] = useState(true);
  const [debugMode, setDebugMode] = useState(false);

  return (
    <SafeAreaView style={settingsStyles.safe}>
      <ScrollView
        style={settingsStyles.container}
        showsVerticalScrollIndicator={false}
      >
        {/* Header */}
        <View style={settingsStyles.header}>
          <Pressable
            style={settingsStyles.backButton}
            onPress={() => router.back()}
          >
            <MaterialCommunityIcons
              name="chevron-left"
              size={24}
              color="#58C95F"
            />
          </Pressable>
          <Text style={settingsStyles.headerTitle}>Settings</Text>
        </View>

        {/* Connection Settings */}
        <View style={settingsStyles.section}>
          <Text style={settingsStyles.sectionTitle}>Connection</Text>

          <View style={settingsStyles.settingItem}>
            <View>
              <Text style={settingsStyles.settingLabel}>Auto-Connect</Text>
              <Text style={settingsStyles.settingDescription}>
                Connect to robot on app launch
              </Text>
            </View>
            <Switch
              value={autoConnect}
              onValueChange={setAutoConnect}
              trackColor={{ false: '#6C7473', true: '#58C95F' }}
              thumbColor={autoConnect ? '#81F295' : '#FFFFFF'}
            />
          </View>
        </View>

        {/* Notification Settings */}
        <View style={settingsStyles.section}>
          <Text style={settingsStyles.sectionTitle}>Notifications</Text>

          <View style={settingsStyles.settingItem}>
            <View>
              <Text style={settingsStyles.settingLabel}>Enable Alerts</Text>
              <Text style={settingsStyles.settingDescription}>
                Receive sensor and system alerts
              </Text>
            </View>
            <Switch
              value={notifications}
              onValueChange={setNotifications}
              trackColor={{ false: '#6C7473', true: '#58C95F' }}
              thumbColor={notifications ? '#81F295' : '#FFFFFF'}
            />
          </View>
        </View>

        {/* System Settings */}
        <View style={settingsStyles.section}>
          <Text style={settingsStyles.sectionTitle}>System</Text>

          <Pressable style={settingsStyles.settingItem}>
            <View>
              <Text style={settingsStyles.settingLabel}>App Version</Text>
              <Text style={settingsStyles.settingDescription}>
                Agribot v2.4.1
              </Text>
            </View>
            <MaterialCommunityIcons
              name="chevron-right"
              size={20}
              color="#6C7473"
            />
          </Pressable>

          <View style={settingsStyles.settingItem}>
            <View>
              <Text style={settingsStyles.settingLabel}>Debug Mode</Text>
              <Text style={settingsStyles.settingDescription}>
                Show debug information
              </Text>
            </View>
            <Switch
              value={debugMode}
              onValueChange={setDebugMode}
              trackColor={{ false: '#6C7473', true: '#58C95F' }}
              thumbColor={debugMode ? '#81F295' : '#FFFFFF'}
            />
          </View>
        </View>

        <View style={settingsStyles.divider} />

        {/* Danger Zone */}
        <View style={settingsStyles.section}>
          <Text style={settingsStyles.sectionTitle}>Danger Zone</Text>

          <Pressable style={settingsStyles.dangerButton}>
            <Text style={settingsStyles.dangerButtonText}>
              Disconnect Robot
            </Text>
          </Pressable>

          <Pressable
            style={[settingsStyles.dangerButton, { marginTop: 8 }]}
            onPress={() => {
              alert('App data will be cleared on next restart');
            }}
          >
            <Text style={settingsStyles.dangerButtonText}>Reset App Data</Text>
          </Pressable>
        </View>

        <View style={{ height: 20 }} />
      </ScrollView>
    </SafeAreaView>
  );
}
