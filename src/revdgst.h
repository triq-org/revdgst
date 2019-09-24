
// Checksum is actually an "LFSR-based Toeplitz hash"
// gen needs to includes the msb if the lfsr is rolling, key is the initial key
void FUNCTION_NAME(DIGEST,ALT)(uint8_t *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    unsigned sum = 0;
    uint8_t xor = 0;
#if ALT & 0x1
    for (int k = bytes - 1; k >= 0; --k) {
#else
    for (int k = 0; k < bytes; ++k) {
#endif
        u_int8_t data = msg[k];
#if ALT & 0x2
        for (int bit = 7; bit >= 0; --bit) {
#else
        for (int bit = 0; bit <= 7; ++bit) {
#endif
            // fprintf(stderr, "key at bit %d : %04x\n", bit, key);
            // if data bit is set then xor with key
            if ((data >> bit) & 1) {
                sum += key;
                xor ^= key;
            }

            // - Galois LFSR -
            // roll the key right (actually the lsb is dropped here)
            // and apply the gen (needs to include the dropped lsb as msb)
#if ALT & 0x4

#if ALT & 0x8
            if (key & 0x80)
            //if ((data >> bit) & 0x01)
                key = (key << 1) ^ gen;
            else
                key = (key << 1);

#else
            if (key & 1)
            //if ((data >> bit) & 0x01)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);
#endif

            // -or- (Fibonacci LFSR)
#else

#if ALT & 0x8
            if (parity(key & gen))
                key = (key << 1) | 1;
            else
                key = (key << 1);
#else
            if (parity(key & gen))
                key = (key >> 1) | (1 << 7);
            else
                key = (key >> 1);
#endif

#endif

            // -and maybe-
            //key ^= (key >> 4);
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

void FUNCTION_NAME(RUNNER,ALT)(struct data *data)
{
        for (int g = 0; g <= 0xff; ++g) {
            for (int k = 0; k <= 0xff; ++k) {
                struct data rd = data[0];
                uint8_t rs;
                uint8_t rx;
                FUNCTION_NAME(DIGEST,ALT)(rd.d, MSG_LEN, g, k, &rs, &rx);
                uint8_t rsx = rs ^ rd.chk;
                uint8_t rxx = rx ^ rd.chk;
                uint8_t rsa = rs + rd.chk;
                uint8_t rxa = rx + rd.chk;
                uint8_t rss = rs - rd.chk;
                uint8_t rxs = rx - rd.chk;
                //printf("g %02x k %02x chk %02x rsx: %02x rxx: %02x rsa: %02x rxa: %02x rss: %02x rxs: %02x\n", g, k, rd.chk, rsx, rxx, rsa, rxa, rss, rxs);
                //printf("rsx: %02x rxx: %02x rsa: %02x rxa: %02x rss: %02x rxs: %02x\n", rsx, rxx, rsa, rxa, rss, rxs);

                int fsx = 0;
                int fxx = 0;
                int fsa = 0;
                int fxa = 0;
                int fss = 0;
                int fxs = 0;

                for (int i = 1; i < DATA_LEN; ++i) {
                    struct data dd = data[i];
                    uint8_t ds;
                    uint8_t dx;
                    FUNCTION_NAME(DIGEST,ALT)(dd.d, MSG_LEN, g, k, &ds, &dx);
                    uint8_t dsx = ds ^ dd.chk;
                    uint8_t dxx = dx ^ dd.chk;
                    uint8_t dsa = ds + dd.chk;
                    uint8_t dxa = dx + dd.chk;
                    uint8_t dss = ds - dd.chk;
                    uint8_t dxs = dx - dd.chk;
                    //printf("dsx: %02x dxx: %02x dsa: %02x dxa: %02x dss: %02x dxs: %02x\n", dsx, dxx, dsa, dxa, dss, dxs);

                    fsx += rsx == dsx;
                    fxx += rxx == dxx;
                    fsa += rsa == dsa;
                    fxa += rxa == dxa;
                    fss += rss == dss;
                    fxs += rxs == dxs;
                }

                if (fsx >= (DATA_LEN - 1)) DONE("sum xor", rsx, 100.0 * fsx / (DATA_LEN - 1));
                if (fsx >= (DATA_LEN - 1)) DONE("sum xor", rsx, 100.0 * fsx / (DATA_LEN - 1));
                if (fxx >= (DATA_LEN - 1)) DONE("xor xor", rxx, 100.0 * fxx / (DATA_LEN - 1));
                if (fsa >= (DATA_LEN - 1)) DONE("sum add", rsa, 100.0 * fsa / (DATA_LEN - 1));
                if (fxa >= (DATA_LEN - 1)) DONE("xor add", rxa, 100.0 * fxa / (DATA_LEN - 1));
                if (fss >= (DATA_LEN - 1)) DONE("sum sub", rss, 100.0 * fss / (DATA_LEN - 1));
                if (fxs >= (DATA_LEN - 1)) DONE("xor sub", rxs, 100.0 * fxs / (DATA_LEN - 1));
            }
        }
}
