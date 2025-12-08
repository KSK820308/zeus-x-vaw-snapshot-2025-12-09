import SwiftUI

struct LEDBar: View {
    @Binding var dbValue: Double
    var segments: Int = 16
    var body: some View {
        let level = max(0, min(127, Int(round((dbValue + 60.0) / 60.0 * 127.0))))
        let lit = Int(round(Double(level) / 127.0 * Double(segments)))
        HStack(spacing: 2) {
            ForEach(0..<segments, id: \.self) { i in
                let t = Double(i) / Double(max(1, segments - 1))
                let c: Color = i < lit ? (t < 0.6 ? Color.green.opacity(0.9) : (t < 0.85 ? Color.yellow.opacity(0.9) : Color.red.opacity(0.9))) : Color.black.opacity(0.2)
                RoundedRectangle(cornerRadius: 2)
                    .fill(c)
                    .frame(width: 6, height: 16)
            }
        }
        .padding(4)
        .background(RoundedRectangle(cornerRadius: 4).fill(Color.black.opacity(0.35)))
    }
}

struct LEDBar_Previews: PreviewProvider {
    static var previews: some View {
        LEDBar(dbValue: .constant(-6))
            .frame(width: 120, height: 24)
            .padding()
    }
}
