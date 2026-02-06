const std = @import("std");

fn qpow(b: u1024, e: u512, m: u512) u512 {
    var res: u1024 = 1;

    const mod: u1024 = m;
    var exp = e;
    var base: u1024 = b % mod;
    while (exp != 0) : (exp >>= 1) {
        if (exp & 1 == 1) {
            res = res * base % mod;
        }
        base = base * base % mod;
    }

    return @as(u512, @truncate(res));
}

const rsa_key = struct {
    p: u512,
    q: u512,
    dp: u512,
    dq: u512,
    qinv: u512,

    fn exptmod(self: *const rsa_key, m: u1024) u1024 {
        const sp = qpow(m, self.dp, self.p);
        const sq = qpow(m, self.dq, self.q);
        const diff = if (sp > sq) sp - sq else sp + (self.p - sq % self.p);
        const h = @as(u1024, self.qinv) * diff % self.p;
        const c = sq + self.q * h;
        return c;
    }
};

// 2-private-103869-t6-WZBC.der
const key = rsa_key{
    .p = 0xff622c3fe95aeddb16ac4e2d69b74749d112c7f5cbe38bcda0ca32ffe811899e4dcc5d319fdc8ee04fa6dfc644ce46b9fb4a6fd3f58dc1385bb6abcd14f35b63,
    .q = 0xce29912a49e3932a36bcb342f545558d1a16db0dc4f916fb0d2ec8fb1a3a05f48aad96ffa1f2437682d729972cb536b951a7562936084d64326bb5bf3146cf95,
    .dp = 0xd250d92592ff96b46d065e7fc078d14bd95ac2ca6bac5503b197754b3795f8dcb88a2ea15679669a9bf2d6670b7cb2b7476a7a361583cc4c87c39c8ac5f5968d,
    .dq = 0x54e3f07ad32178d52598fe84fb95051bfbaf0ee78d5781eee74f7feeecae7aec391a4d3c1581df8b26d111202177cb3d3fbd5fb69dc72eed05b3e16cd80e193d,
    .qinv = 0x36e662287fee5bbc30bd24ec6a5aa53649cc1ae7700e4f6ca10e612d405ae48386f8e4b6a1174d3811fd49ab967ec1cf1bcb9e6f8fd7225ad68f0949b2a3d21c,
};

pub export fn z_rsassa_pkcs1v15_sign(in: [*]const u8, inlen: usize, signature: *[128]u8) void {
    // PKCS#1 v1.5 Signature scheme

    var sha1_hash: [20]u8 = undefined;
    std.crypto.hash.Sha1.hash(in[0..inlen], &sha1_hash, .{});

    // PKCS#1 DigestInfo structure wrap sha1_hash
    const dlen = 35;
    var digest_info: [35]u8 = undefined;
    @memcpy(digest_info[0..15], &[15]u8{0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14});
    @memcpy(digest_info[15..], sha1_hash[0..20]);

    // PKCS#1 Padding for digest_info
    var encoded_message: [128]u8 = undefined;
    encoded_message[0] = 0;
    encoded_message[1] = 1;
    @memset(encoded_message[2..128-dlen-1], 0xff);
    encoded_message[128-dlen-1] = 0;
    @memcpy(encoded_message[128-dlen..], digest_info[0..]);

    // RSA signature (1024bit) on encoded_message
    const m = std.mem.readInt(u1024, &encoded_message, .big);
    const c = key.exptmod(m);
    std.mem.writeInt(u1024, signature, c, .big);

    return;
}
