import SwiftUI

struct MasterLEDMeter: View {
    @Binding var lValue127: Int
    @Binding var rValue127: Int
    var body: some View {
        GeometryReader { geo in
            let h = geo.size.height
            let w = geo.size.width
            let segments = 20
            let spacing: CGFloat = 4
            let segH = max(6, (h - CGFloat(segments - 1) * spacing) / CGFloat(segments))
            let litL = Int(round(Double(lValue127) / 127.0 * Double(segments)))
            let litR = Int(round(Double(rValue127) / 127.0 * Double(segments)))
            HStack(spacing: 10) {
                VStack(spacing: spacing) {
                    ForEach(0..<segments, id: \.self) { i in
                        let t = Double(i) / Double(max(1, segments - 1))
                        let c: Color = i < litL ? (t < 0.8 ? Color.green.opacity(0.9) : (t < 0.95 ? Color.yellow.opacity(0.9) : Color.red.opacity(0.9))) : Color.black.opacity(0.25)
                        RoundedRectangle(cornerRadius: 2).fill(c).frame(height: segH)
                    }
                }
                VStack(spacing: spacing) {
                    ForEach(0..<segments, id: \.self) { i in
                        let t = Double(i) / Double(max(1, segments - 1))
                        let c: Color = i < litR ? (t < 0.8 ? Color.green.opacity(0.9) : (t < 0.95 ? Color.yellow.opacity(0.9) : Color.red.opacity(0.9))) : Color.black.opacity(0.25)
                        RoundedRectangle(cornerRadius: 2).fill(c).frame(height: segH)
                    }
                }
            }
            .frame(width: w, height: h)
            .padding(.horizontal, 4)
            .background(
                ZStack {
                    RoundedRectangle(cornerRadius: 6).fill(Color.black.opacity(0.3))
                    // странични тик‑марки
                    HStack {
                        VStack(spacing: spacing/2) {
                            ForEach(0..<segments*2, id: \.self) { _ in
                                Rectangle().fill(Color.gray.opacity(0.5)).frame(width: 2, height: 2)
                            }
                        }
                        Spacer(minLength: 0)
                        VStack(spacing: spacing/2) {
                            ForEach(0..<segments*2, id: \.self) { _ in
                                Rectangle().fill(Color.gray.opacity(0.5)).frame(width: 2, height: 2)
                            }
                        }
                    }
                    .padding(.horizontal, 2)
                    // централни дБ надписи
                    VStack {
                        Spacer()
                        Text("0")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.8))
                        Spacer()
                        Text("-3")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.6))
                        Spacer()
                        Text("-6")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.6))
                        Spacer()
                        Text("-10")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.6))
                        Spacer()
                        Text("-15")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.6))
                        Spacer()
                        Text("-20")
                            .font(.system(size: 9, weight: .semibold, design: .monospaced))
                            .foregroundColor(.white.opacity(0.6))
                        Spacer()
                        HStack {
                            Text("L")
                                .font(.system(size: 9, weight: .bold))
                                .foregroundColor(.green.opacity(0.9))
                            Spacer()
                            Text("R")
                                .font(.system(size: 9, weight: .bold))
                                .foregroundColor(.green.opacity(0.9))
                        }
                        .padding(.horizontal, 6)
                    }
                }
            )
            .shadow(color: Color.green.opacity(0.25), radius: 6)
        }
    }
}

struct MasterLEDMeter_Previews: PreviewProvider {
    static var previews: some View {
        MasterLEDMeter(lValue127: .constant(100), rValue127: .constant(72))
            .frame(width: 50, height: 230)
            .padding()
    }
}
