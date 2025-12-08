import SwiftUI

struct BusSendKnob: View {
    @Binding var value127: Int
    var tint: Color
    var body: some View {
        VStack(spacing: 4) {
            GeometryReader { geo in
                let size = min(geo.size.width, geo.size.height)
                let radius = size * 0.45
                let angle = Double(value127) / 127.0 * 270.0 - 135.0
                let segments = 24
                let step = 270.0 / Double(segments)
                ZStack {
                    Circle()
                        .fill(LinearGradient(colors: [Color(white: 0.18), Color(white: 0.10)], startPoint: .top, endPoint: .bottom))
                        .overlay(Circle().stroke(Color.black.opacity(0.4), lineWidth: 1))
                    ForEach(0..<segments, id: \.self) { i in
                        let segAngle = -135.0 + step * Double(i)
                        let lit = segAngle <= -135.0 + Double(value127) / 127.0 * 270.0
                        Capsule()
                            .fill(lit ? Color(hex: 0x00D47D) : Color.black.opacity(0.3))
                            .frame(width: size * 0.035, height: size * 0.07)
                            .offset(y: -radius)
                            .rotationEffect(.degrees(segAngle))
                    }
                    Path { p in
                        let w = size * 0.05
                        let h = size * 0.08
                        p.move(to: CGPoint(x: size/2, y: size*0.10))
                        p.addLine(to: CGPoint(x: size/2 - w/2, y: size*0.10 + h))
                        p.addLine(to: CGPoint(x: size/2 + w/2, y: size*0.10 + h))
                        p.closeSubpath()
                    }
                    .fill(Color.white)
                    .rotationEffect(.degrees(angle))
                }
            }
            .frame(width: 30, height: 30)
            .gesture(DragGesture(minimumDistance: 0).onChanged { g in
                let dy = -g.translation.height
                let new = Double(value127) + dy / 3.0
                value127 = min(127, max(0, Int(round(new))))
            })
            Text("\(value127)")
                .font(.system(size: 9, weight: .semibold, design: .monospaced))
                .foregroundColor(.secondary)
        }
    }
}

struct BusSendKnob_Previews: PreviewProvider {
    static var previews: some View {
        BusSendKnob(value127: .constant(64), tint: .green)
            .frame(width: 60, height: 80)
    }
}
