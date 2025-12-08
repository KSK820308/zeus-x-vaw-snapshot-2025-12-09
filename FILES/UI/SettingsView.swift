import SwiftUI

struct SettingsView: View {
    @State private var enableLowLatency = true
    @State private var sampleRate: Double = 48000
    @State private var bufferSize: Int = 256
    var body: some View {
        Form {
            Toggle("Low-latency", isOn: $enableLowLatency)
            HStack {
                Text("Sample Rate")
                Spacer()
                Text(String(format: "%.0f Hz", sampleRate))
            }
            Slider(value: $sampleRate, in: 44100...96000, step: 100)
            HStack {
                Text("Buffer Size")
                Spacer()
                Text("\(bufferSize) frames")
            }
            Slider(value: Binding(
                get: { Double(bufferSize) },
                set: { bufferSize = Int($0) }
            ), in: 64...1024, step: 64)
        }
        .padding()
    }
}

struct SettingsView_Previews: PreviewProvider {
    static var previews: some View {
        SettingsView()
            .frame(width: 600, height: 400)
    }
}
