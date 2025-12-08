import SwiftUI

struct LiveView: View {
    @ObservedObject var midi: MIDIManager
    @StateObject private var mixer = Mixer.default16()
    var body: some View {
        VStack(spacing: 0) {
            MixerBox(mixer: mixer)
                .padding(.horizontal, 6)
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        }
        .onAppear {
            VoiceActions.ensureUserSlots()
            SF2Engine.shared.setup(mixer: mixer, midi: midi)
        }
    }
}

struct LiveView_Previews: PreviewProvider {
    static var previews: some View {
        LiveView(midi: MIDIManager())
            .frame(minWidth: 1200, minHeight: 700)
    }
}

struct InfoBox: View {
    let title: String
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(title)
                .font(.system(size: 12, weight: .heavy))
                .foregroundColor(ZXTheme.labelText)
            Spacer()
        }
        .padding(8)
        .frame(maxWidth: .infinity)
        .frame(maxHeight: .infinity)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.black.opacity(0.08))
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .stroke(ZXTheme.panelBorder, lineWidth: 1)
                )
        )
    }
}
