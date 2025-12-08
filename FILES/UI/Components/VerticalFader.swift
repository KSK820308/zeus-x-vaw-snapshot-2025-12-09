import SwiftUI

struct VerticalFader: View {
    @Binding var dbValue: Double
    var height: CGFloat = 240
    var showScale: Bool = false
    @State private var editingValue: String = ""
    @State private var showEditor: Bool = false
    var body: some View {
        GeometryReader { geo in
            let h = geo.size.height
            let w: CGFloat = 48
            let handleHeight: CGFloat = 24
            let value127 = max(0, min(127, Int(round((dbValue + 60.0) / 60.0 * 127.0))))
            let centerY: CGFloat = {
                let normalized = CGFloat(value127) / 127.0
                let travel = h - handleHeight
                return (1 - normalized) * travel + handleHeight/2
            }()
            ZStack {
                let track = RoundedRectangle(cornerRadius: 8)
                track
                    .fill(
                        LinearGradient(
                            colors: [Color(red:0.14, green:0.14, blue:0.16), Color(red:0.09, green:0.09, blue:0.11)],
                            startPoint: .top, endPoint: .bottom
                        )
                    )
                    .overlay(track.stroke(Color.white.opacity(0.10), lineWidth: 1))
                    .frame(width: w, height: h)
                let grooveWidth = max(10, w * 0.32)
                RoundedRectangle(cornerRadius: 6)
                    .fill(LinearGradient(colors: [Color(white: 0.10), Color(white: 0.06)], startPoint: .top, endPoint: .bottom))
                    .frame(width: grooveWidth, height: h - 8)
                    .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.white.opacity(0.10), lineWidth: 1))
                    .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.black.opacity(0.35), lineWidth: 0.5).blur(radius: 0.3))
                    .shadow(color: Color.black.opacity(0.35), radius: 2, y: 1)
                FaderHandle(value: value127, width: w - 10, height: handleHeight)
                    .position(x: w/2, y: centerY)
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onChanged { g in
                                let locY = min(max(0, g.location.y), h)
                                let raw = 1 - ((locY - handleHeight/2) / max(1, h - handleHeight))
                                let mapped = (raw * 127).rounded()
                                let next = max(0, min(127, Int(mapped)))
                                dbValue = Double(next)/127.0*60.0 - 60.0
                            }
                    )
                    .onTapGesture(count: 2) {
                        editingValue = String(value127)
                        showEditor = true
                    }
                    .popover(isPresented: $showEditor) {
                        VStack(spacing: 8) {
                            Text("Volume (0..127)")
                            TextField("127", text: $editingValue)
                                .textFieldStyle(RoundedBorderTextFieldStyle())
                                .frame(width: 120)
                                .onSubmit {
                                    if let v = Int(editingValue) {
                                        let cl = max(0, min(127, v))
                                        dbValue = Double(cl)/127.0*60.0 - 60.0
                                    }
                                    showEditor = false
                                }
                        }
                        .padding()
                    }
            }
        }
        .frame(width: 48, height: height)
    }
}

private struct FaderHandle: View {
    let value: Int
    let width: CGFloat
    let height: CGFloat
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 6)
                .fill(LinearGradient(colors: [Color(white: 0.28), Color(white: 0.12)], startPoint: .top, endPoint: .bottom))
                .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.white.opacity(0.20), lineWidth: 1))
                .shadow(color: .black.opacity(0.6), radius: 6, y: 2)
                .shadow(color: Color.black.opacity(0.25), radius: 1.5, y: 0.5)
            Text("\(value)")
                .font(.system(size: 13, weight: .heavy, design: .rounded))
                .foregroundStyle(.white)
                .allowsHitTesting(false)
        }
        .frame(width: width, height: height)
    }
}

private struct ScaleView: View {
    @Binding var dbValue: Double
    var height: CGFloat
    var body: some View {
        ZStack(alignment: .leading) {
            RoundedRectangle(cornerRadius: 4)
                .fill(Color.gray.opacity(0.2))
            GeometryReader { geo in
                let h = geo.size.height
                let ticks = stride(from: 0, through: 60, by: 3).map { Int($0) }
                ForEach(ticks, id: \.self) { t in
                    let y = h * (1 - CGFloat(Double(t) / 60.0))
                    Group {
                        Rectangle().fill(Color.gray.opacity(0.7)).frame(width: t % 6 == 0 ? 16 : 8, height: 1)
                            .offset(x: 4, y: y)
                        if t % 12 == 0 {
                            Text(t == 0 ? "0" : "-\(t)")
                                .font(.system(size: 8, weight: .regular, design: .monospaced))
                                .foregroundColor(ZXTheme.scaleText)
                                .offset(x: 20, y: y - 6)
                        }
                    }
                }
            }
        }
    }
}

struct VerticalFader_Previews: PreviewProvider {
    static var previews: some View {
        VerticalFader(dbValue: .constant(-12))
            .padding()
            .frame(width: 36, height: 280)
    }
}
