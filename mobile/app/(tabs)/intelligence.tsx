import { Text, View } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';

export default function IntelligenceScreen() {
  return (
    <SafeAreaView style={{ flex: 1, backgroundColor: '#070A0A' }}>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center', paddingHorizontal: 20 }}>
        <Text style={{ color: '#F1F6F5', fontSize: 28, fontWeight: '700' }}>Intelligence</Text>
        <Text style={{ color: '#AAB5B4', marginTop: 8, textAlign: 'center' }}>
          AI mission analytics and recommendations will appear here.
        </Text>
      </View>
    </SafeAreaView>
  );
}
