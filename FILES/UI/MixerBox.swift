import SwiftUI

enum MixerPage: Int, CaseIterable, Identifiable {
    case page1 = 1
    case page2 = 2
    var id: Int { rawValue }
    var title: String { self == .page1 ? "PAGE 1" : "PAGE 2" }
}

struct MixerBox: View {
    @ObservedObject var mixer: Mixer
    @State private var page: MixerPage = .page1
    @State private var busChannels: [MixerChannel] = Mixer.bus8()
    @State private var selectedID: UUID? = nil
    var body: some View {
        GeometryReader { geo in
            let spacing: CGFloat = 6
            let totalVisible = page == .page1 ? visibleChannels.count : visibleChannels.count + 1
            let stripWidth = max(72, (geo.size.width - CGFloat(max(0, totalVisible - 1)) * spacing) / CGFloat(max(1, totalVisible)))
            let contentPadding: CGFloat = 16
            let headerHeight: CGFloat = 24
            let headerToStripsSpacing: CGFloat = 8
            let minBoxesHeight: CGFloat = 160
            let availableContentHeight = max(0, geo.size.height - contentPadding)
            let spaceAfterHeader = max(0, availableContentHeight - headerHeight - headerToStripsSpacing)
            let desiredStripHeight = max(220, availableContentHeight * 0.45)
            let stripHeight = min(desiredStripHeight, max(0, spaceAfterHeader - minBoxesHeight))
            let boxesHeight = max(minBoxesHeight, spaceAfterHeader - stripHeight)
            VStack(spacing: 8) {
                HStack(spacing: 8) {
                    PageButton(title: MixerPage.page1.title, selected: page == .page1) { page = .page1 }
                    PageButton(title: MixerPage.page2.title, selected: page == .page2) { page = .page2 }
                    Spacer()
                }
                .padding(.horizontal, 8)
                VStack(spacing: 0) {
                    HStack(spacing: spacing) {
                        ForEach(visibleChannels) { ch in
                            BusStripView(channel: ch, selected: selectedID == ch.id, onSelect: { selectedID = ch.id })
                                .frame(width: stripWidth, height: stripHeight)
                        }
                        if page == .page2 {
                            BusStripView(channel: mixer.master, selected: selectedID == mixer.master.id, onSelect: { selectedID = mixer.master.id })
                                .frame(width: stripWidth, height: stripHeight)
                        }
                    }
                    HStack(spacing: 8) {
                        VoicePanel(stripWidth: stripWidth, mixer: mixer, selectedID: selectedID)
                        InfoBox(title: "CONTROLS")
                        InfoBox(title: "STYLE")
                    }
                    .frame(maxWidth: .infinity)
                    .frame(height: boxesHeight)
                }
            }
            .padding(8)
            .frame(maxWidth: .infinity, alignment: .topLeading)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.black.opacity(0.08))
                    .overlay(RoundedRectangle(cornerRadius: 12).stroke(ZXTheme.panelBorder, lineWidth: 1))
            )
        }
    }

    private var visibleChannels: [MixerChannel] {
        switch page {
        case .page1:
            return mixer.channels
        case .page2:
            return busChannels
        }
    }
}

private struct VoicePanel: View {
    let stripWidth: CGFloat
    let mixer: Mixer
    let selectedID: UUID?
    var body: some View {
        ZStack {
            // centered VOICE title
            VStack {
                Text("VOICE")
                    .font(.system(size: 12, weight: .heavy))
                    .foregroundColor(ZXTheme.labelText)
                    .frame(maxWidth: .infinity)
                    .multilineTextAlignment(.center)
                    .padding(.top, 10)
                Spacer()
            }
            HStack(spacing: 8) {
                VoiceInnerBox(title: "ZEUS-X", width: max(60, stripWidth - 12)) {
                    VoiceActions.openZXI(mixer: mixer, selectedID: selectedID)
                }
                Spacer(minLength: 0)
                VoiceMiddleBox(mixer: mixer, selectedID: selectedID, width: max(60, stripWidth - 12))
                    .frame(maxWidth: .infinity)
                Spacer(minLength: 0)
                VoiceInnerBox(title: "USER", width: max(60, stripWidth - 12)) {
                    VoiceActions.saveSound(from: mixer)
                }
            }
            .padding(8)
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

private struct VoiceMiddleBox: View {
    let mixer: Mixer
    let selectedID: UUID?
    let width: CGFloat
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.black.opacity(0.06))
                .overlay(
                    RoundedRectangle(cornerRadius: 10)
                        .stroke(ZXTheme.panelBorder, lineWidth: 1)
                )
        }
        .frame(maxWidth: .infinity)
        .frame(maxHeight: .infinity)
    }
}

private struct VoiceInnerBox: View {
    let title: String
    let width: CGFloat
    let action: () -> Void
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.black.opacity(0.06))
                .overlay(
                    RoundedRectangle(cornerRadius: 10)
                        .stroke(ZXTheme.panelBorder, lineWidth: 1)
                )
            VStack(spacing: 0) {
                Text(title)
                    .font(.system(size: 12, weight: .heavy))
                    .foregroundColor(ZXTheme.labelText)
                    .frame(maxWidth: .infinity)
                    .multilineTextAlignment(.center)
                    .padding(.top, 10)
                Spacer()
                VoiceButton(title: title == "ZEUS-X" ? "OPEN SOUND" : "SAVE SOUND", action: action)
            }
            .padding(.horizontal, 4)
        }
        .frame(width: width)
        .frame(maxHeight: .infinity)
    }
}


private struct VoiceButton: View {
    let title: String
    let action: () -> Void
    var body: some View {
        Text(title)
            .font(.system(size: 11, weight: .semibold))
            .foregroundColor(.white)
            .frame(maxWidth: .infinity)
            .frame(height: 22)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(ZXTheme.slotBG)
                    .overlay(RoundedRectangle(cornerRadius: 6).stroke(ZXTheme.panelBorder, lineWidth: 1))
            )
            .onTapGesture { action() }
            .padding(.bottom, 8)
    }
}

private struct PageButton: View {
    let title: String
    let selected: Bool
    let action: () -> Void
    var body: some View {
        Text(title)
            .font(.system(size: 12, weight: .heavy))
            .foregroundColor(selected ? .white : .secondary)
            .padding(.horizontal, 10)
            .frame(height: 24)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(selected ? Color.black.opacity(0.25) : Color.clear)
                    .overlay(RoundedRectangle(cornerRadius: 6).stroke(ZXTheme.panelBorder, lineWidth: 1))
            )
            .onTapGesture { action() }
    }
}
