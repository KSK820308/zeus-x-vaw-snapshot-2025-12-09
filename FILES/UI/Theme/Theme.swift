import SwiftUI

enum ZXTheme {
    static let panelBG = Color.black.opacity(0.88)
    static let panelBorder = Color.white.opacity(0.08)
    static let slotBG = Color.black.opacity(0.55)
    static let slotText = Color.white
    static let scaleText = Color.white.opacity(0.75)
    static let faderTrack = LinearGradient(
        colors: [Color(red:0.14, green:0.14, blue:0.16), Color(red:0.09, green:0.09, blue:0.11)],
        startPoint: .top, endPoint: .bottom
    )
    static let faderHandleTop = Color(white: 0.88)
    static let faderHandleBottom = Color(white: 0.65)
    static let muteBG = Color.gray.opacity(0.25)
    static let soloBG = Color.gray.opacity(0.25)
    static let muteActive = Color(hex: 0xE23B3B)
    static let soloActive = Color(hex: 0x00D47D)
    static let labelText = Color.white
    static let labelInst = Color(hex: 0x00C2B8)
    static let topBadgeGreen = Color(hex: 0x00C2B8)
    static let topBadgePink = Color(hex: 0x6E44FF)
    static let rightAccent = Color(hex: 0x3B82F6)
    static let lowerAccent = Color(hex: 0xF59E0B)
    static let mixAccent = Color(hex: 0xC4A000)
    static let frogGreen = Color(hex: 0x39FF14)
    static let labelMaster = Color(hex: 0xEF4444)
}

extension Color {
    init(hex: UInt32) {
        let r = Double((hex >> 16) & 0xFF) / 255.0
        let g = Double((hex >> 8) & 0xFF) / 255.0
        let b = Double(hex & 0xFF) / 255.0
        self = Color(red: r, green: g, blue: b)
    }
}
