import SwiftUI

struct StudioView: View {
    var midi: MIDIManager? = nil
    @State private var selection: Int = 0
    var body: some View {
        VStack(spacing: 0) {
            Picker("Studio", selection: $selection) {
                Text("Autosampling").tag(0)
                Text("Mapping").tag(1)
                Text("Sound Design").tag(2)
                Text("Styles").tag(3)
                Text("Audio Phrases").tag(4)
                Text("Plugins").tag(5)
            }
            .pickerStyle(SegmentedPickerStyle())
            .padding()

            Group {
                if selection == 0 { SF2ConverterView() }
                else if selection == 1 { MappingView(midi: midi) }
                else if selection == 2 { StudioSectionView(title: "Sound Design") }
                else if selection == 3 { StudioSectionView(title: "Styles") }
                else if selection == 4 { StudioSectionView(title: "Audio Phrases") }
                else { PluginsView() }
            }
        }
    }
}

struct StudioSectionView: View {
    let title: String
    var body: some View {
        ZStack {
            Rectangle()
                .fill(Color.gray.opacity(0.08))
                .ignoresSafeArea()
            Text(title)
                .font(.title2)
        }
    }
}

struct PluginsView: View {
    @State private var plugins: [VST3Plugin] = PluginHost.scanVST3Plugins()
    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text("VST3 Plugins")
                    .font(.title3)
                Spacer()
                Button("Rescan") { plugins = PluginHost.scanVST3Plugins() }
            }
            .padding(.horizontal)
            List(plugins) { p in
                VStack(alignment: .leading) {
                    Text(p.name).font(.headline)
                    Text(p.path).font(.caption).foregroundColor(.secondary)
                }
            }
        }
    }
}

struct StudioView_Previews: PreviewProvider {
    static var previews: some View {
        StudioView()
            .frame(minWidth: 1200, minHeight: 700)
    }
}
