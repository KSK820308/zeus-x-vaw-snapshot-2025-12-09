import SwiftUI

struct VerticalLEDBar: View {
    @Binding var dbValue: Double
    var segments: Int = 24
    var body: some View {
        GeometryReader { geo in
            let level = max(0, min(127, Int(round((dbValue + 60.0) / 60.0 * 127.0))))
            let lit = Int(round(Double(level) / 127.0 * Double(segments)))
            let spacing: CGFloat = 2
            let segH = max(4, (geo.size.height - CGFloat(segments - 1) * spacing) / CGFloat(segments))
            VStack(spacing: spacing) {
                ForEach(0..<segments, id: \.self) { i in
                    let t = Double(i) / Double(max(1, segments - 1))
                    let c: Color = i < lit ? (t < 0.6 ? Color.green.opacity(0.9) : (t < 0.85 ? Color.yellow.opacity(0.9) : Color.red.opacity(0.9))) : Color.black.opacity(0.2)
                    Capsule()
                        .fill(c)
                        .frame(width: 4, height: segH)
                }
            }
        }
        .frame(width: 8)
        .background(RoundedRectangle(cornerRadius: 4).fill(Color.black.opacity(0.35)))
        .rotationEffect(.degrees(180))
    }
}

struct VerticalLEDBar_Previews: PreviewProvider {
    static var previews: some View {
        VerticalLEDBar(dbValue: .constant(-12))
            .frame(height: 230)
            .padding()
    }
}
