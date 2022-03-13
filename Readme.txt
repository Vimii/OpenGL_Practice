操作方法

WASDQE　　 移動
左SHIFT　　高速移動
マウス　   視点移動
R	   マウス表示
C	　 マウス非表示
F	   デバッグウィンドウ閉
G	   デバッグウィンドウ開

------------------------------------------------------------

実行環境 : Visual Studio 2019 x64

OpenGL使用バージョン: 4.0

ソリューションのプロパティから以下の操作を行ってください。

構成:すべての構成　プラットフォーム: x64

C/C++ → 追加のインクルードディレクトリ

vendor;$(SolutionDir)Dependences\GLFW\include;$(SolutionDir)Dependences\GLEW\include;

リンカー　→　全般　→　追加のライブラリディレクトリ

$(SolutionDir)Dependences\GLFW\lib-vc2019;$(SolutionDir)Dependences\GLEW\lib\Release\x64

リンカー　→　入力　→　追加の依存ファイル

glfw3.lib;opengl32.lib;User32.lib;Gdi32.lib;Shell32.lib;glew32s.lib;


環境構築は以下の動画を参考にしたので参照してください。
https://youtu.be/OR4fNpBjmq8
