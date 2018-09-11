#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ZMIENNE GLOBALNE
char *input = NULL;
int input_length = 0;
int input_length_with_pad = 0;
long long msg_length = 0;
long long msg_length_with_pad = 0;
int blocks;
int r, e;

int *words = NULL;
int words_count = 0;
int constants[16] = {0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
	0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C, 0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917};
int chain[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};
int salt = 0;
int permutation[14][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}
};
int v[16] = {0};
long licznik = 0;

int rotacja_prawo(int x, int n);
void G(int block, int r, int i, int *v, int a, int b, int c, int d);

int main(int argc, char** argv)
{
    if (argc == 2) {
        // LICZE LICZBE ZNAKOW NA WEJSCIU
        char *c = argv[1];
        while (*c != '\0') {
            input_length++;
            c++;
        }

        // PAMIEC NA WEJSCIE
        input = (char *)realloc(input, input_length * sizeof(char));

        // PRZEPISANIE WEJSCIA Z PARAMETRU DO TABLICY
        strncpy(input, argv[1], input_length);

        // DLUGOSC W BITACH
        msg_length = input_length * 4;
    }

    // PADDING
    input_length_with_pad = input_length;

    // DOPEŁNIJ DO 8 ZNAKÓW ABY UŻYĆ STROTOUL
	int to_add = 8 - (input_length % 8);
    for (; to_add > 0; to_add--) {
        input_length_with_pad++;
        input = (char*)realloc(input, input_length_with_pad * sizeof(char));
        input[input_length_with_pad - 1] = '0';
    }

    // DLUGOSC WIADOMOSCI Z PADDINGIEM, MSG_LENGTH ZOSTAWIAM W SPOKOJU
	msg_length_with_pad = msg_length;

	// ZAALOKUJ PAMIĘĆ NA SŁOWA
    words_count = input_length_with_pad / 8;
    words = (int*)malloc(words_count * sizeof(int));

    // INPUT NA int PO 8 ZNAKOW
    char *temp_word = (char*)malloc(9 * sizeof(char));
    temp_word[8] = '\0';

    int c = 0;
    int d = 0;
    while (c < input_length_with_pad) {
        strncpy(temp_word, &input[d * 8], 8);
        c += 8;

        words[d] = strtoul(temp_word, '\0', 16);
        d++;
    }
    free(temp_word);

    // USTAWIENIE NASTEPNEGO BITU PO WIADOMOSCI NA 1
    int which_bit =	msg_length % 32;
	int which_word = msg_length / 32;

	// JEZELI SLOWO NA KTORYM ZAPISUJEMY 1 NIE ISTNIEJE TO DODAC
	if ((which_word + 1) > words_count) {
		words_count++;
        words = realloc(words, words_count * sizeof(int));
        words[words_count - 1] = 0;
	}

	// USTAW BIT PADDINGU
	words[which_word] = words[which_word] | (1 << (31 - which_bit));

	// NASZA WIADOMOSC Z PADDINGIEM MA DLUGOSC O 1 WIEKSZA
	msg_length_with_pad++;

	// DOKLADAJ KOLEJNE SLOWA DOPOKI NIE DOJDZIEMY DO OSTATNIEGO BITU PRZED DLUGOSCIA WIADOMOSCI
	while (msg_length_with_pad % 512 != 448) {
		if (msg_length_with_pad % 32 == 0) {
			// DODAJ NA KONCU SLOWO 0x00000000
			words_count++;
            words = realloc(words, words_count * sizeof(int));
            words[words_count - 1] = 0;
		}

        // NASZA WIADOMOSC Z PADDINGIEM MA DLUGOSC O 1 WIEKSZA
		msg_length_with_pad++;
	}

	// USTAWIENIAE BITU PRZED DLUGOSCIA WIADOMOSCI - USTAWIAM JEDYNKE
	words[words_count - 1] = words[words_count - 1] | 1;

    // ZAPISYWANIE WIADOMOSCI NA OSTATNICH 64 BITACH - BARDZIEJ ZNACZACA CZESC msg_length
    words_count += 1;
    words = (int*)realloc(words, words_count * sizeof(int));
    words[words_count - 1] = 0;

    // ZAPISYWANIE WIADOMOSCI NA OSTATNICH 64 BITACH - MNIEJ ZNACZACA CZESC msg_length
    words_count += 1;
    words = (int*)realloc(words, words_count * sizeof(int));
    words[words_count - 1] = (int)msg_length;

    blocks = words_count / 16;

    int na_ilu_blokach_mozna_zapisac = (int)ceil((float)msg_length / 512);

    for (e = 1; e <= blocks; e++) {
        if (e > na_ilu_blokach_mozna_zapisac) {
            // BLOK BEZ WIADOMOSCI
            licznik = 0;
        }
        else if (e < na_ilu_blokach_mozna_zapisac) {
            // BLOK KTORYS Z KOLEI Z WIADOMOSCIA NA CALYM BLOKU
            licznik = e * 512;
        }
        else {
            // BLOK OSTATNI Z WIADOMOSCIA
            licznik = msg_length;
        }

        v[0] = chain[0];
        v[1] = chain[1];
        v[2] = chain[2];
        v[3] = chain[3];
        v[4] = chain[4];
        v[5] = chain[5];
        v[6] = chain[6];
        v[7] = chain[7];
        v[8] = salt ^ constants[0];
        v[9] = salt ^ constants[1];
        v[10] = salt ^ constants[2];
        v[11] = salt ^ constants[3];
        v[12] = licznik ^ constants[4];
        v[13] = licznik ^ constants[5];
        v[14] = constants[6];
        v[15] = constants[7];

		for (r = 0; r < 14; r++) {
			G(e, r, 0, v, 0, 4, 8, 12);
			G(e, r, 1, v, 1, 5, 9, 13);
			G(e, r, 2, v, 2, 6, 10, 14);
			G(e, r, 3, v, 3, 7, 11, 15);
			G(e, r, 4, v, 0, 5, 10, 15);
			G(e, r, 5, v, 1, 6, 11, 12);
			G(e, r, 6, v, 2, 7, 8, 13);
			G(e, r, 7, v, 3, 4, 9, 14);
		}

		chain[0] = chain[0] ^ salt ^ v[0] ^ v[8];
		chain[1] = chain[1] ^ salt ^ v[1] ^ v[9];
		chain[2] = chain[2] ^ salt ^ v[2] ^ v[10];
		chain[3] = chain[3] ^ salt ^ v[3] ^ v[11];
		chain[4] = chain[4] ^ salt ^ v[4] ^ v[12];
		chain[5] = chain[5] ^ salt ^ v[5] ^ v[13];
		chain[6] = chain[6] ^ salt ^ v[6] ^ v[14];
		chain[7] = chain[7] ^ salt ^ v[7] ^ v[15];
    }

    for (d = 0; d < 8; d++) {
        printf("%.8X", chain[d]);
    }

    return 0;
}

int rotacja_prawo(int x, int n) {
    int y, z, g;
	y = (x >> n) & ~(-1 << (32 - n));
    z = x << (32 - n);
    g = y | z;
    return g;
}

void G(int block, int r, int i, int *v, int a, int b, int c, int d) {
	int word_offset = (block - 1) * 16;

    if (r > 9) {
        r = r - 10;
    }

	v[a] = v[a] + v[b] + (words[ word_offset + permutation[ r ][ 2 * i ] ] ^ constants[ permutation[ r ][ 2 * i + 1 ] ]);
	v[d] = rotacja_prawo(v[d] ^ v[a], 16);
	v[c] = v[c] + v[d];
	v[b] = rotacja_prawo(v[b] ^ v[c], 12);
	v[a] = v[a] + v[b] + (words[ word_offset + permutation[ r ][ 2 * i + 1 ] ] ^ constants[ permutation[ r ][ 2 * i ] ]);
	v[d] = rotacja_prawo(v[d] ^ v[a], 8);
	v[c] = v[c] + v[d];
	v[b] = rotacja_prawo(v[b] ^ v[c], 7);
}
