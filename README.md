# fts5_mecab
fts5_mecab.so provides mecab tokenizer for sqlite3 fts5.

## build environment
1. install sqlite3 from source
```
mkdir -p $HOME/usr/src
cd $HOME/usr/src
wget https://www.sqlite.org/2019/sqlite-autoconf-3290000.tar.gz
tar zxvf sqlite-autoconf-3290000.tar.gz
cd sqlite-autoconf-3290000
./configure --enable-fts5 --prefix=$HOME/usr
make
make install
```

2. install mecab from source
```
cd $HOME/usr/src
git clone https://github.com/taku910/mecab.git
cd mecab/mecab
./configure --prefix=$HOME/usr --with-charset=UTF8
make
make install
```

3. install mecab-ipadic
```
cd $HOME/usr/src
wget 'https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7MWVlSDBCSXZMTXM' -O mecab-ipadic-2.7.0-20070801.tar.gz
tar xvzf mecab-ipadic-2.7.0-20070801.tar.gz
cd mecab-ipadic-2.7.0-20070801/
./configure --with-mecab-config=$HOME/usr/bin/mecab-config --with-charset=UTF8 –enable-utf8-only --prefix=$HOME/usr
make
make install
```

4. install mecab-ipadic-NEologd and edit mecabrc (Optional)
```
cd $HOME/usr/src
git clone --depth 1 https://github.com/neologd/mecab-ipadic-neologd.git
cd mecab-ipadic-neologd/
./bin/install-mecab-ipadic-neologd -y -n -u --prefix $HOME/usr/lib/mecab/dic/mecab-ipadic-neologd
cp -a $HOME/usr/etc/mecabrc{,_backup}
sed -i -e "s#$HOME/usr/lib/mecab/dic/ipadic#$HOME/usr/lib/mecab/dic/mecab-ipadic-neologd#g" $HOME/usr/etc/mecabrc
```

## compile and execute

Case without macro.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab
$ LD_LIBRARY_PATH=$HOME/usr/lib sqlite3
sqlite> .load ./fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab');
```

Case with parameters 'v' or 'vv'.
This requires compile option '-DDEBUG'.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab -DDEBUG
$ LD_LIBRARY_PATH=$HOME/usr/lib sqlite3
sqlite> .load /PATH/TO/fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab vv');
```

Case with parameters 'stop789'.
This requires compile option '-DSTOP789'.
If you use stop789 option, the token which has posid 7,8,9 will be discarded.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab -DSTOP789
$ LD_LIBRARY_PATH=$HOME/usr/lib sqlite3
sqlite> .load /PATH/TO/fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab stop789');
```

## bash alias (Optional)

Add a line into .bashrc.

```
alias sqlite3='LD_LIBRARY_PATH=$HOME/usr/lib sqlite3 -cmd ".load $HOME/usr/lib/fts5_mecab"'
```
