import SwiftUI

enum AppMode: String, CaseIterable, Identifiable {
    case live = "Live"
    case studio = "Studio"
    case settings = "Settings"
    var id: String { rawValue }
}

struct MainView: View {
    @State private var mode: AppMode = .live
    @StateObject private var midi = MIDIManager()
    var body: some View {
        VStack(spacing: 0) {
            TopBar(midi: midi)
            Picker("Mode", selection: $mode) {
                ForEach(AppMode.allCases) { m in
                    Text(m.rawValue).tag(m)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .padding()

            switch mode {
            case .live:
                LiveView(midi: midi)
            case .studio:
                StudioView(midi: midi)
            case .settings:
                SettingsView()
            }
        }
    }
}

struct MainView_Previews: PreviewProvider {
    static var previews: some View {
        MainView()
            .frame(minWidth: 1200, minHeight: 700)
    }
}
