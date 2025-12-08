import SwiftUI

struct KnobView: View {
    @Binding var value: Double // -1...1
    var label: String = ""
    var tint: Color = ZXTheme.topBadgeGreen
    var body: some View {
        VStack(spacing: 6) {
            ZStack {
                Circle()
                    .fill(Color.gray.opacity(0.2))
                    .overlay(Circle().stroke(Color.black.opacity(0.2), lineWidth: 1))
                // arc
                Circle()
                    .trim(from: 0.0, to: 1.0)
                    .stroke(tint.opacity(0.8), lineWidth: 4)
                    .padding(6)
                // pointer
                Rectangle()
                    .fill(Color.white)
                    .frame(width: 2, height: 10)
                    .offset(y: -16)
                    .rotationEffect(.degrees(90 + value * 135))
            }
            .frame(width: 48, height: 48)
            .gesture(DragGesture(minimumDistance: 0).onChanged { g in
                let dy = -g.translation.height
                let delta = dy / 150
                value = max(-1, min(1, value + delta))
            })
            Text(label.isEmpty ? String(format: "%.1f", value) : label)
                .font(.caption2)
                .foregroundColor(.secondary)
        }
    }
}

struct KnobView_Previews: PreviewProvider {
    static var previews: some View {
        KnobView(value: .constant(0.0), label: "Pan", tint: ZXTheme.topBadgePink)
            .padding()
            .frame(width: 100, height: 120)
    }
}
