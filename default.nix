with import <nixpkgs> { };
stdenv.mkDerivation {
  name = "esesc";
  src = fetchFromGitHub {
    owner = "masc-ucsc";
    repo = "esesc";
    rev = "2d18b03c23057a3b21cf166f788e8e57a28f5745";
    sha256 = "sha256:0zvp080x7q5aid59li4fjqaq7g8nlm7j0srmnyrqbkqryh8l4phh";
  };

  # build inputs only for building (not runtime)
  nativeBuildInputs = [ cmake bison flex pkg-config ];

  buildInputs = [ ncurses xorg.libX11 zlib python27 perl pixman glib ];

  postPatch = ''
    		# We want to replace all the hardcoded /bin/ls
    		FILES_WITH_BIN_LS="
    			emul/qemu_riscv/Makefile.target
    			emul/qemu_riscv/qemu-doc.texi
    			emul/qemu_mipsr6/Makefile.target
    			emul/qemu_mipsr6/qemu-doc.texi
    		"
    	    for f in $FILES_WITH_BIN_LS
    	    do
    	    	# I have no clue why qemu tries to do `@cp /bin/ls $@`
          		substituteInPlace $f --replace "/bin/ls" "${coreutils}/bin/ls"
        	done
    	'';

  installPhase = ''
    mkdir -p $out/bin
    cp main/esesc $out/bin
  '';
}
