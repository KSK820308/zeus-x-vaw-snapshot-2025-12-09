import SwiftUI

struct BusSendPanel: View {
    @ObservedObject var channel: MixerChannel
    var accent: Color
    var body: some View {
        VStack(spacing: 8) {
            ForEach(0..<4, id: \.self) { i in
                HStack(spacing: 12) {
                    Menu {
                        ForEach(1...8, id: \.self) { b in
                            Button("Bus \(b)") { channel.busTargets[i] = b }
                        }
                    } label: {
                        Text("Bus \(channel.busTargets[i])")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(ZXTheme.slotText)
                            .frame(width: 68, height: 24)
                            .background(ZXTheme.slotBG)
                            .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent, lineWidth: 1))
                            .clipShape(RoundedRectangle(cornerRadius: 6))
                    }
                    Spacer(minLength: 12)
                    BusSendKnob(value127: Binding(get: { channel.busSends127[i] }, set: { channel.busSends127[i] = $0 }), tint: accent)
                    TextField("0", text: Binding(
                        get: { String(channel.busSends127[i]) },
                        set: { v in
                            let n = Int(v) ?? channel.busSends127[i]
                            channel.busSends127[i] = max(0, min(127, n))
                        }
                    ))
                    .frame(width: 44)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .font(.system(size: 12, weight: .semibold, design: .monospaced))
                }
            }
        }
        .padding(8)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(ZXTheme.panelBG)
                .overlay(RoundedRectangle(cornerRadius: 12).stroke(accent, lineWidth: 1))
        )
    }
}

struct BusSendPanel_Previews: PreviewProvider {
    static var previews: some View {
        BusSendPanel(channel: MixerChannel(name: "Inst 1"), accent: .green)
            .frame(width: 160)
    }
}
