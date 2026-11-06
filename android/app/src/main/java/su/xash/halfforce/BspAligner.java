package su.xash.halfforce;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Realinha em multiplos de 4 bytes os blocos (lumps) de mapas BSP do Half-Life (versao 29/30).
 *
 * O Xash3D em ARM de 32 bits cai com "Bus error" (acesso desalinhado) ao carregar mapas cujos blocos
 * nao comecam em enderecos multiplos de 4. O conteudo dos blocos NAO muda, so a posicao deles no arquivo.
 * Usa so java.io/java.nio (nao depende do Android).
 */
final class BspAligner {

    static final int LUMP_COUNT = 15;
    static final int HEADER_SIZE = 4 + 8 * LUMP_COUNT;

    private static final int COPY_BUFFER = 64 * 1024;

    private BspAligner() {
    }

    /** Cabecalho ja lido e validado. */
    private static final class Header {
        final int version;
        final int[] ofs = new int[LUMP_COUNT];
        final int[] len = new int[LUMP_COUNT];

        Header(int version) {
            this.version = version;
        }
    }

    private static Header readHeader(RandomAccessFile f) throws IOException {
        long size = f.length();
        if (size < HEADER_SIZE) {
            return null;
        }

        byte[] raw = new byte[HEADER_SIZE];
        f.seek(0);
        f.readFully(raw);
        ByteBuffer b = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);

        int version = b.getInt();
        if (version != 29 && version != 30) {
            return null; // outro formato: nao mexe
        }

        Header h = new Header(version);
        for (int i = 0; i < LUMP_COUNT; i++) {
            h.ofs[i] = b.getInt();
            h.len[i] = b.getInt();
            if (h.ofs[i] < 0 || h.len[i] < 0 || (long) h.ofs[i] + h.len[i] > size) {
                return null; // arquivo suspeito: nao mexe
            }
        }
        return h;
    }

    /** true se algum bloco com dados comeca em endereco nao multiplo de 4. */
    static boolean needsFix(File file) throws IOException {
        RandomAccessFile f = new RandomAccessFile(file, "r");
        try {
            Header h = readHeader(f);
            if (h == null) {
                return false;
            }
            for (int i = 0; i < LUMP_COUNT; i++) {
                if (h.len[i] > 0 && (h.ofs[i] & 3) != 0) {
                    return true;
                }
            }
            return false;
        } finally {
            f.close();
        }
    }

    /**
     * Regrava o mapa com os blocos alinhados. Escreve num arquivo temporario, confere o resultado
     * bloco a bloco e so entao troca o original. Devolve true se o arquivo foi alterado.
     */
    static boolean fix(File file) throws IOException {
        File dir = file.getAbsoluteFile().getParentFile();
        File tmp = new File(dir, file.getName() + ".hfalign.tmp");

        RandomAccessFile src = new RandomAccessFile(file, "r");
        RandomAccessFile dst = null;
        boolean replaced = false;

        try {
            Header h = readHeader(src);
            if (h == null) {
                return false;
            }

            boolean bad = false;
            for (int i = 0; i < LUMP_COUNT; i++) {
                if (h.len[i] > 0 && (h.ofs[i] & 3) != 0) {
                    bad = true;
                }
            }
            if (!bad) {
                return false;
            }

            if (dir.getUsableSpace() < file.length() + 1024 * 1024) {
                throw new IOException("sem espaco livre para corrigir " + file.getName());
            }

            // ordem em que os blocos aparecem no arquivo original (por offset; empate pelo indice)
            Integer[] order = new Integer[LUMP_COUNT];
            for (int i = 0; i < LUMP_COUNT; i++) {
                order[i] = i;
            }
            final Header hh = h;
            java.util.Arrays.sort(order, new java.util.Comparator<Integer>() {
                @Override
                public int compare(Integer a, Integer b) {
                    if (hh.ofs[a] != hh.ofs[b]) {
                        return hh.ofs[a] < hh.ofs[b] ? -1 : 1;
                    }
                    return a < b ? -1 : (a > b ? 1 : 0);
                }
            });

            if (tmp.exists() && !tmp.delete()) {
                throw new IOException("nao consegui apagar " + tmp.getName());
            }
            dst = new RandomAccessFile(tmp, "rw");
            dst.setLength(0);
            dst.write(new byte[HEADER_SIZE]); // cabecalho provisorio

            int[] newOfs = new int[LUMP_COUNT];
            byte[] buf = new byte[COPY_BUFFER];

            for (int k = 0; k < LUMP_COUNT; k++) {
                int i = order[k];
                padTo4(dst);
                newOfs[i] = (int) dst.getFilePointer();
                copy(src, h.ofs[i], h.len[i], dst, buf);
            }
            padTo4(dst);

            // cabecalho definitivo
            ByteBuffer nb = ByteBuffer.allocate(HEADER_SIZE).order(ByteOrder.LITTLE_ENDIAN);
            nb.putInt(h.version);
            for (int i = 0; i < LUMP_COUNT; i++) {
                nb.putInt(newOfs[i]);
                nb.putInt(h.len[i]);
            }
            dst.seek(0);
            dst.write(nb.array());

            // conferencia: cada bloco tem que estar alinhado e igual ao original
            for (int i = 0; i < LUMP_COUNT; i++) {
                if (h.len[i] > 0 && (newOfs[i] & 3) != 0) {
                    throw new IOException("verificacao falhou (alinhamento) em " + file.getName());
                }
                if (!sameBytes(src, h.ofs[i], dst, newOfs[i], h.len[i], buf)) {
                    throw new IOException("verificacao falhou (conteudo) em " + file.getName());
                }
            }

            dst.close();
            dst = null;
            src.close();
            src = null;

            // troca atomica (rename sobrescreve o original no mesmo sistema de arquivos)
            if (!tmp.renameTo(file)) {
                throw new IOException("nao consegui substituir " + file.getName());
            }
            replaced = true;
            return true;
        } finally {
            if (dst != null) {
                try {
                    dst.close();
                } catch (IOException ignored) {
                }
            }
            if (src != null) {
                try {
                    src.close();
                } catch (IOException ignored) {
                }
            }
            if (!replaced && tmp.exists()) {
                tmp.delete();
            }
        }
    }

    private static void padTo4(RandomAccessFile f) throws IOException {
        long pos = f.getFilePointer();
        int pad = (int) ((4 - (pos & 3)) & 3);
        if (pad > 0) {
            f.write(new byte[pad]);
        }
    }

    private static void copy(RandomAccessFile from, long fromOfs, long len, RandomAccessFile to, byte[] buf)
            throws IOException {
        from.seek(fromOfs);
        long left = len;
        while (left > 0) {
            int n = (int) Math.min(buf.length, left);
            from.readFully(buf, 0, n);
            to.write(buf, 0, n);
            left -= n;
        }
    }

    private static boolean sameBytes(RandomAccessFile a, long aOfs, RandomAccessFile b, long bOfs, long len,
                                     byte[] bufA) throws IOException {
        byte[] bufB = new byte[bufA.length];
        long done = 0;
        while (done < len) {
            int n = (int) Math.min(bufA.length, len - done);
            a.seek(aOfs + done);
            a.readFully(bufA, 0, n);
            b.seek(bOfs + done);
            b.readFully(bufB, 0, n);
            for (int i = 0; i < n; i++) {
                if (bufA[i] != bufB[i]) {
                    return false;
                }
            }
            done += n;
        }
        return true;
    }
}
