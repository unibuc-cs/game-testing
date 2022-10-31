namespace TestsPriorityApp
{
    partial class BlueprintTesterConfig
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.dataGridView1 = new System.Windows.Forms.DataGridView();
            this.button1 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.Run = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.BlueprintName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.MinutesSinceModified = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Category = new System.Windows.Forms.DataGridViewComboBoxColumn();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).BeginInit();
            this.SuspendLayout();
            // 
            // dataGridView1
            // 
            this.dataGridView1.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView1.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.Run,
            this.BlueprintName,
            this.MinutesSinceModified,
            this.Category});
            this.dataGridView1.Location = new System.Drawing.Point(33, 32);
            this.dataGridView1.Name = "dataGridView1";
            this.dataGridView1.RowHeadersWidth = 62;
            this.dataGridView1.RowTemplate.Height = 28;
            this.dataGridView1.Size = new System.Drawing.Size(690, 869);
            this.dataGridView1.TabIndex = 0;
            this.dataGridView1.CellContentClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_CellContentClick);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(749, 61);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(363, 54);
            this.button1.TabIndex = 1;
            this.button1.Text = "Run Tests";
            this.button1.UseVisualStyleBackColor = true;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(753, 404);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(388, 90);
            this.button2.TabIndex = 2;
            this.button2.Text = "Configure clusters";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // Run
            // 
            this.Run.HeaderText = "Run";
            this.Run.MinimumWidth = 8;
            this.Run.Name = "Run";
            this.Run.Width = 50;
            // 
            // BlueprintName
            // 
            this.BlueprintName.HeaderText = "BlueprintName";
            this.BlueprintName.MinimumWidth = 8;
            this.BlueprintName.Name = "BlueprintName";
            this.BlueprintName.ReadOnly = true;
            this.BlueprintName.Width = 250;
            // 
            // MinutesSinceModified
            // 
            this.MinutesSinceModified.HeaderText = "Minutes since modified";
            this.MinutesSinceModified.MinimumWidth = 8;
            this.MinutesSinceModified.Name = "MinutesSinceModified";
            this.MinutesSinceModified.ReadOnly = true;
            this.MinutesSinceModified.Width = 150;
            // 
            // Category
            // 
            this.Category.HeaderText = "Category";
            this.Category.Items.AddRange(new object[] {
            "Gameplay",
            "Animation",
            "Physics",
            "Rendering",
            "Sound",
            "Systems"});
            this.Category.MinimumWidth = 8;
            this.Category.Name = "Category";
            this.Category.ReadOnly = true;
            this.Category.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.Category.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.Automatic;
            this.Category.Width = 150;
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(859, 153);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(122, 26);
            this.textBox1.TabIndex = 3;
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(749, 153);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(88, 20);
            this.label1.TabIndex = 4;
            this.label1.Text = "Time to run";
            this.label1.Click += new System.EventHandler(this.label1_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(1005, 156);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(34, 20);
            this.label2.TabIndex = 5;
            this.label2.Text = "min";
            this.label2.Click += new System.EventHandler(this.label2_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(749, 317);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(113, 20);
            this.label3.TabIndex = 6;
            this.label3.Text = "Current Status";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.SystemColors.HotTrack;
            this.label4.Location = new System.Drawing.Point(889, 316);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(46, 20);
            this.label4.TabIndex = 7;
            this.label4.Text = "IDLE";
            // 
            // BlueprintTesterConfig
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1155, 945);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.dataGridView1);
            this.Name = "BlueprintTesterConfig";
            this.Text = "BlueprintTesterConfig";
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView dataGridView1;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.DataGridViewCheckBoxColumn Run;
        private System.Windows.Forms.DataGridViewTextBoxColumn BlueprintName;
        private System.Windows.Forms.DataGridViewTextBoxColumn MinutesSinceModified;
        private System.Windows.Forms.DataGridViewComboBoxColumn Category;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
    }
}