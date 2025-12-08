import SwiftUI

struct PanKnob: View {
    @Binding var pan: Int
    var tint: Color
    @State private var showEditor: Bool = false
    @State private var editingValue: String = "0"
    var body: some View {
        VStack(spacing: 6) {
            GeometryReader { geo in
                let size = min(geo.size.width, geo.size.height)
                let radius = size * 0.45
                let angle = Double(pan) / 64.0 * 135.0
                let segments = 24
                let step = 270.0 / Double(segments)
                ZStack {
                    Circle()
                        .fill(LinearGradient(colors: [Color(white: 0.18), Color(white: 0.10)], startPoint: .top, endPoint: .bottom))
                        .overlay(Circle().stroke(Color.black.opacity(0.4), lineWidth: 1))
                        .shadow(color: Color.black.opacity(0.5), radius: 3, y: 1)
                    ForEach(0..<segments, id: \.self) { i in
                        let segAngle = -135.0 + step * Double(i)
                        let centerGap = abs(segAngle) <= step / 2.0
                        let rightLit = segAngle > 0 && Double(pan) > 0 && segAngle <= Double(pan) / 64.0 * 135.0
                        let leftLit = segAngle < 0 && Double(pan) < 0 && segAngle >= Double(pan) / 64.0 * 135.0
                        let lit = !centerGap && (rightLit || leftLit)
                        Capsule()
                            .fill(lit ? Color(hex: 0x00D47D) : Color.black.opacity(0.3))
                            .frame(width: size * 0.04, height: size * 0.08)
                            .offset(y: -radius)
                            .rotationEffect(.degrees(segAngle))
                    }
                    Path { p in
                        let w = size * 0.06
                        let h = size * 0.10
                        p.move(to: CGPoint(x: size/2, y: size*0.08))
                        p.addLine(to: CGPoint(x: size/2 - w/2, y: size*0.08 + h))
                        p.addLine(to: CGPoint(x: size/2 + w/2, y: size*0.08 + h))
                        p.closeSubpath()
                    }
                    .fill(Color.white)
                    .rotationEffect(.degrees(angle))
                }
            }
            .frame(width: 48, height: 48)
            .gesture(DragGesture(minimumDistance: 0).onChanged { g in
                let dy = -g.translation.height
                let dx = g.translation.width
                let delta = (dy - dx * 0.25) / 150.0
                let new = Double(pan) + delta * 64.0
                pan = min(64, max(-64, Int(round(new))))
            })
            .onTapGesture(count: 2) {
                editingValue = String(pan)
                showEditor = true
            }
            .popover(isPresented: $showEditor) {
                VStack(spacing: 8) {
                    Text("Pan (-64..64)")
                    TextField("0", text: $editingValue)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 120)
                        .onSubmit {
                            if let v = Int(editingValue) {
                                pan = min(64, max(-64, v))
                            }
                            showEditor = false
                        }
                }
                .padding()
            }
            HStack(spacing: 6) {
                Text("Pan")
                    .font(.caption2)
                    .foregroundColor(.secondary)
                TextField("0", text: Binding(
                    get: { String(pan) },
                    set: { v in
                        let n = Int(v) ?? pan
                        pan = min(64, max(-64, n))
                    }
                ))
                .frame(width: 40)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .font(.caption2)
            }
        }
    }
}

struct PanKnob_Previews: PreviewProvider {
    static var previews: some View {
        PanKnob(pan: .constant(0), tint: ZXTheme.topBadgeGreen)
            .padding()
            .frame(width: 100, height: 120)
    }
}
