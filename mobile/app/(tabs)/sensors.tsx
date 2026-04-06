import React, { useState } from 'react';
import {
  SafeAreaView,
  ScrollView,
  View,
  Text,
  Pressable,
} from 'react-native';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import { sensorsStyles } from '@/styles/sensors.styles';

type SensorIconName = 'thermometer' | 'water-percent' | 'video' | 'flask' | 'map-marker' | 'water-opacity';

const sensorsList = [
  {
    id: 1,
    name: 'Temperature',
    icon: 'thermometer' as SensorIconName,
    status: 'Active Node',
    value: '24.5°C',
    enabled: true,
    iconColor: '#4A9AFF',
  },
  {
    id: 2,
    name: 'Humidity',
    icon: 'water-percent' as SensorIconName,
    status: 'Live Feed',
    value: '62%',
    enabled: true,
    iconColor: '#58C95F',
  },
  {
    id: 3,
    name: 'Camera',
    icon: 'video' as SensorIconName,
    status: 'Spectroscopy STREAMING Active',
    value: '',
    enabled: true,
    iconColor: '#4A9AFF',
  },
  {
    id: 4,
    name: 'pH',
    icon: 'flask' as SensorIconName,
    status: 'Subsurface Probe',
    value: '6.5',
    enabled: true,
    iconColor: '#58C95F',
  },
  {
    id: 5,
    name: 'GPS',
    icon: 'map-marker' as SensorIconName,
    status: 'Signal Strong Precision Fix',
    value: '',
    enabled: true,
    iconColor: '#4A9AFF',
  },
  {
    id: 6,
    name: 'Soil Moisture',
    icon: 'water-opacity' as SensorIconName,
    status: 'Standby',
    value: '--',
    enabled: false,
    iconColor: '#999999',
  },
];

export default function SensorsScreen() {
  const [sensors, setSensors] = useState(sensorsList);

  const toggleSensor = (id: number) => {
    setSensors(sensors.map(s =>
      s.id === id ? { ...s, enabled: !s.enabled } : s
    ));
  };

  return (
    <SafeAreaView style={sensorsStyles.safe}>
      <ScrollView
        style={sensorsStyles.container}
        showsVerticalScrollIndicator={false}
      >
        {/* Header */}
        <View style={sensorsStyles.headerSection}>
          <Text style={sensorsStyles.diagnosticLabel}>
            DIAGNOSTIC SHELL V4.2
          </Text>
          <Text style={sensorsStyles.pageTitle}>Sensors</Text>
          <View style={sensorsStyles.titleUnderline} />
        </View>

        {/* Sensors Grid */}
        <View style={sensorsStyles.sensorsGrid}>
          {sensors.map(sensor => (
            <View key={sensor.id} style={sensorsStyles.sensorCard}>
              {/* Icon */}
              <View
                style={[
                  sensorsStyles.sensorIconContainer,
                  { backgroundColor: sensor.iconColor + '20' },
                ]}
              >
                <MaterialCommunityIcons
                  name={sensor.icon}
                  size={28}
                  color={sensor.enabled ? sensor.iconColor : '#6C7473'}
                />
              </View>

              {/* Content */}
              <View style={sensorsStyles.sensorContent}>
                <Text style={sensorsStyles.sensorName}>{sensor.name}</Text>
                <View style={{ flexDirection: 'row', alignItems: 'center' }}>
                  <Text
                    style={[
                      sensorsStyles.sensorStatus,
                      sensor.enabled && sensorsStyles.sensorStatusActive,
                    ]}
                  >
                    {'• ' + sensor.status}
                  </Text>
                  {sensor.value && (
                    <Text style={sensorsStyles.sensorValue}>
                      {sensor.value}
                    </Text>
                  )}
                </View>
              </View>

              {/* Toggle Switch */}
              <Pressable
                style={[
                  sensorsStyles.sensorToggle,
                  sensor.enabled
                    ? sensorsStyles.toggleOn
                    : sensorsStyles.toggleOff,
                ]}
                onPress={() => toggleSensor(sensor.id)}
              >
                <View
                  style={[
                    sensorsStyles.toggleCircle,
                    sensor.enabled
                      ? sensorsStyles.toggleCircleRight
                      : sensorsStyles.toggleCircleLeft,
                  ]}
                />
              </Pressable>
            </View>
          ))}
        </View>

        {/* Add New Sensor Button */}
        <Pressable style={sensorsStyles.addSensorButton}>
          <MaterialCommunityIcons
            name="plus-circle"
            size={20}
            color="#070A0A"
          />
          <Text style={sensorsStyles.addSensorText}>Add New Sensor</Text>
        </Pressable>

        {/* Stats Footer */}
        <View style={sensorsStyles.statsSection}>
          <View style={sensorsStyles.statCard}>
            <Text style={sensorsStyles.statLabel}>HEALTH INDEX</Text>
            <Text style={sensorsStyles.statValue}>98.4%</Text>
          </View>
          <View style={sensorsStyles.statCard}>
            <Text style={sensorsStyles.statLabel}>UPTIME</Text>
            <Text style={sensorsStyles.statValue}>142h</Text>
          </View>
        </View>

        <View style={{ height: 20 }} />
      </ScrollView>
    </SafeAreaView>
  );
}

