import SwiftUI

struct BusStripView: View {
    @ObservedObject var channel: MixerChannel
    var selected: Bool = false
    var onSelect: (() -> Void)? = nil
    @State private var showDetails = false
    // velocity follows channel state
    @State private var showBusPopover = false
    var body: some View {
        VStack(spacing: 6) {
            TopBadge(symbol: channel.badgeSymbol, imagePath: channel.badgeImagePath, text: channel.topTitle, tint: channel.isMaster ? ZXTheme.labelMaster : channel.accent, selected: selected)
            PanKnob(pan: $channel.pan, tint: channel.isMaster ? ZXTheme.labelMaster : channel.accent)
            HStack(spacing: 6) {
                VStack(alignment: .leading, spacing: 6) {
                    HStack(spacing: 6) {
                        VerticalLEDBar(dbValue: $channel.dbVolume)
                        VerticalFader(dbValue: Binding(get: { channel.dbVolume }, set: { channel.dbVolume = $0 }), height: 220, showScale: false)
                        if !channel.isMaster && channel.topTitle != "BUS" {
                            RightControlsColumn(channel: channel, accent: channel.accent, showBusPopover: $showBusPopover)
                                .frame(maxWidth: .infinity)
                                .frame(height: 220, alignment: .top)
                                .padding(.top, 2)
                        }
                    }
                    if !channel.isMaster {
                        HStack(spacing: 6) {
                            RectToggle(title: "M", isOn: $channel.mute, activeColor: ZXTheme.muteActive)
                            RectToggle(title: "S", isOn: $channel.solo, activeColor: ZXTheme.soloActive)
                        }
                    }
                }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.vertical, 4)
            if !channel.bottomTitle.isEmpty {
                BottomLabel(text: channel.bottomTitle, color: channel.accent, selected: selected)
            }
        }
        .padding(8)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(ZXTheme.panelBG)
                .overlay(RoundedRectangle(cornerRadius: 10).stroke(ZXTheme.panelBorder, lineWidth: 1))
                .overlay(
                    Group {
                        if selected {
                            RoundedRectangle(cornerRadius: 10)
                                .stroke(Color.white.opacity(0.9), lineWidth: 1)
                                .blur(radius: 1)
                            RoundedRectangle(cornerRadius: 11)
                                .stroke(Color.white.opacity(0.35), lineWidth: 3)
                                .blur(radius: 1.5)
                            Rectangle()
                                .fill(Color.white.opacity(0.9))
                                .frame(height: 3)
                                .blur(radius: 0.5)
                                .padding(.horizontal, 8)
                                .alignmentGuide(.bottom) { d in d[.bottom] }
                                .frame(maxHeight: .infinity, alignment: .bottom)
                        }
                    }
                )
        )
        .zIndex(selected ? 1 : 0)
        .onTapGesture { onSelect?() }
        .onChange(of: channel.dbVolume) { SF2Engine.shared.applyMixerState() }
        .onChange(of: channel.pan) { SF2Engine.shared.applyMixerState() }
        .onChange(of: channel.mute) { SF2Engine.shared.applyMixerState() }
        .onChange(of: channel.solo) { SF2Engine.shared.applyMixerState() }
    }
}

private struct TopBadge: View {
    var symbol: String
    var imagePath: String?
    var text: String?
    var tint: Color
    var selected: Bool = false
    var body: some View {
        ZStack {
            let shape = RoundedRectangle(cornerRadius: 8)
            shape
                .fill(Color.clear)
                .frame(height: 30)
                .overlay(shape.stroke(tint, lineWidth: 1))
            if let text, !text.isEmpty {
                Text(text)
                    .font(.system(size: 13, weight: .heavy, design: .rounded))
                    .foregroundColor(selected ? ZXTheme.frogGreen : .white)
                    .padding(.horizontal, 8)
            } else if let imagePath, let nsImage = NSImage(contentsOfFile: (imagePath as NSString).expandingTildeInPath) {
                Image(nsImage: nsImage)
                    .resizable()
                    .scaledToFit()
                    .frame(height: 20)
            } else {
                Image(systemName: symbol)
                    .font(.system(size: 18, weight: .medium))
                    .foregroundColor(.white)
            }
        }
    }
}

private struct ValueSlot: View {
    var value: Double
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 4)
                .fill(ZXTheme.slotBG)
                .frame(width: 48, height: 22)
            Text(String(format: "%.1f", value))
                .font(.system(size: 11, weight: .semibold, design: .monospaced))
                .foregroundColor(ZXTheme.slotText)
        }
    }
}

private struct RectToggle: View {
    let title: String
    @Binding var isOn: Bool
    var activeColor: Color
    var body: some View {
        Text(title)
            .font(.system(size: 12, weight: .bold))
            .foregroundColor(.white)
            .frame(width: 28, height: 22)
            .background(isOn ? activeColor : Color.gray.opacity(0.35))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.black.opacity(0.25), lineWidth: 1))
            .onTapGesture { isOn.toggle() }
    }
}

private struct BottomLabel: View {
    let text: String
    let color: Color
    var selected: Bool = false
    var body: some View {
        ZStack {
            let shape = RoundedRectangle(cornerRadius: 8)
            shape
                .fill(Color.clear)
                .overlay(shape.stroke(color, lineWidth: 1))
            Text(text)
                .font(.system(size: 13, weight: .heavy, design: .rounded))
                .foregroundColor(selected ? ZXTheme.frogGreen : ZXTheme.labelText)
        }
        .frame(maxWidth: .infinity)
        .frame(height: 26)
    }
}

private struct BadgeText: View {
    let text: String
    init(_ t: String) { self.text = t }
    var body: some View {
        Text(text)
            .font(.system(size: 11, weight: .bold))
            .foregroundColor(.white)
            .padding(.horizontal, 6)
            .frame(height: 22)
            .background(Color.gray.opacity(0.35))
            .clipShape(RoundedRectangle(cornerRadius: 4))
    }
}

private struct ParamControl: View {
    let title: String
    let value: Int
    let accent: Color
    let action: () -> Void
    var body: some View {
        HStack(spacing: 6) {
            Text(title)
                .font(.system(size: 11, weight: .heavy))
                .foregroundColor(.white)
                .frame(width: 44, height: 20, alignment: .leading)
                .background(Color.gray.opacity(0.35))
                .clipShape(RoundedRectangle(cornerRadius: 4))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent, lineWidth: 1))
            IntValueSlot(value: value)
                .frame(width: 44, height: 20)
        }
    }
}

private struct IntValueSlot: View {
    var value: Int
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 4)
                .fill(ZXTheme.slotBG)
            Text(String(value))
                .font(.system(size: 11, weight: .semibold, design: .monospaced))
                .foregroundColor(ZXTheme.slotText)
        }
    }
}

private struct EditableIntSlot: View {
    @Binding var value: Int
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 4)
                .fill(ZXTheme.slotBG)
            TextField("0", text: Binding(
                get: { String(value) },
                set: { s in
                    let v = Int(s.filter { $0.isNumber }) ?? value
                    value = max(0, min(127, v))
                }
            ))
            .font(.system(size: 11, weight: .semibold, design: .monospaced))
            .foregroundColor(ZXTheme.slotText)
            .multilineTextAlignment(.center)
        }
    }
}

private struct MIDIParamsWideRow: View {
    @ObservedObject var channel: MixerChannel
    var body: some View {
        HStack(spacing: 4) {
            HStack(spacing: 6) {
                Text("MSB")
                    .font(.system(size: 10, weight: .heavy))
                    .foregroundColor(.white)
                    .frame(width: 28, height: 18, alignment: .center)
                    .background(Color.gray.opacity(0.35))
                    .clipShape(RoundedRectangle(cornerRadius: 4))
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(channel.accent, lineWidth: 1))
                EditableIntSlot(value: $channel.msb)
                    .frame(width: 34, height: 18)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            HStack(spacing: 6) {
                Text("LSB")
                    .font(.system(size: 10, weight: .heavy))
                    .foregroundColor(.white)
                    .frame(width: 28, height: 18, alignment: .center)
                    .background(Color.gray.opacity(0.35))
                    .clipShape(RoundedRectangle(cornerRadius: 4))
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(channel.accent, lineWidth: 1))
                EditableIntSlot(value: $channel.lsb)
                    .frame(width: 34, height: 18)
            }
            .frame(maxWidth: .infinity, alignment: .center)
            HStack(spacing: 6) {
                Text("PC")
                    .font(.system(size: 10, weight: .heavy))
                    .foregroundColor(.white)
                    .frame(width: 28, height: 18, alignment: .center)
                    .background(Color.gray.opacity(0.35))
                    .clipShape(RoundedRectangle(cornerRadius: 4))
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(channel.accent, lineWidth: 1))
                EditableIntSlot(value: $channel.pc)
                    .frame(width: 34, height: 18)
            }
            .frame(maxWidth: .infinity, alignment: .trailing)
        }
    }
}

struct BusStripView_Previews: PreviewProvider {
    static var previews: some View {
        BusStripView(channel: MixerChannel(name: "Inst 1"))
            .frame(width: 160, height: 360)
            .padding()
    }
}
private struct BusButton: View {
    var accent: Color
    var title: String
    var action: () -> Void
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.system(size: 11, weight: .heavy))
                .foregroundColor(.white)
                .padding(.horizontal, 8)
                .frame(maxWidth: .infinity)
                .frame(height: 22)
                .background(ZXTheme.slotBG)
                .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent, lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 6))
        }
        .buttonStyle(.plain)
    }
}

private struct RightControlsColumn: View {
    @ObservedObject var channel: MixerChannel
    var accent: Color
    @Binding var showBusPopover: Bool
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            BusButton(accent: accent, title: "BUS") { showBusPopover = true }
                .popover(isPresented: $showBusPopover) {
                    BusSendPanel(channel: channel, accent: accent)
                        .frame(width: 320)
                        .padding()
                }
            BusButton(accent: accent, title: "MONO") { }
            BusButton(accent: accent, title: channel.velOn ? "VEL ON" : "VEL OFF") { channel.velOn.toggle() }
            if (1...6).contains(channel.channelNumber) {
                BusButton(accent: accent, title: "TERCA") { }
            }
            CompactSignedParamRow(title: "TRANS", accent: accent, value: $channel.transpose, minValue: -12, maxValue: 12)
            CompactSignedParamRow(title: "OCTAV", accent: accent, value: $channel.octave, minValue: -3, maxValue: 3)
            CompactParamRow(title: "MSB", accent: accent, value: $channel.msb)
            CompactParamRow(title: "LSB", accent: accent, value: $channel.lsb)
            CompactParamRow(title: "PC", accent: accent, value: $channel.pc)
            Spacer()
        }
    }
}

private struct CompactParamRow: View {
    let title: String
    let accent: Color
    @Binding var value: Int
    var body: some View {
        HStack(spacing: 6) {
            Text(title)
                .font(.system(size: 10, weight: .heavy))
                .foregroundColor(.white)
                .frame(width: 28, height: 18, alignment: .center)
                .background(Color.gray.opacity(0.35))
                .clipShape(RoundedRectangle(cornerRadius: 4))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent, lineWidth: 1))
            EditableIntSlot(value: $value)
                .frame(width: 34, height: 18)
        }
    }
}

private struct CompactSignedParamRow: View {
    let title: String
    let accent: Color
    @Binding var value: Int
    let minValue: Int
    let maxValue: Int
    var body: some View {
        HStack(spacing: 6) {
            Text(title)
                .font(.system(size: 10, weight: .heavy))
                .foregroundColor(.white)
                .frame(width: 38, height: 18, alignment: .center)
                .background(Color.gray.opacity(0.35))
                .clipShape(RoundedRectangle(cornerRadius: 4))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent, lineWidth: 1))
            EditableSignedIntSlot(value: $value, minValue: minValue, maxValue: maxValue)
                .frame(width: 44, height: 18)
        }
    }
}

private struct EditableSignedIntSlot: View {
    @Binding var value: Int
    let minValue: Int
    let maxValue: Int
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 4)
                .fill(ZXTheme.slotBG)
            TextField("0", text: Binding(
                get: { String(value) },
                set: { s in
                    let filtered = s.filter { $0.isNumber || $0 == "-" }
                    let v = Int(filtered) ?? value
                    value = Swift.max(minValue, Swift.min(maxValue, v))
                }
            ))
            .font(.system(size: 11, weight: .semibold, design: .monospaced))
            .foregroundColor(ZXTheme.slotText)
            .multilineTextAlignment(.center)
        }
    }
}
